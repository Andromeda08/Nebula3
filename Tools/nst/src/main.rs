use std::env;
use colored::Colorize;
use glob;

struct Params {
    src:     std::path::PathBuf,
    bin_dir: std::path::PathBuf,
    debug:   bool,
}

fn get_key_value(key: &str, args: &Vec<String>) -> Result<String, String> {
    if args.contains(&String::from(key)) {
        let arg_idx = match args.iter().position(|a| a == &String::from(key)) {
            None => return Err(String::from(format!("Failed to find the argument index for {0}", key))),
            Some(v) => v
        };
        if arg_idx + 1 < args.len() {
            let param = &args[arg_idx + 1];
            return match param.starts_with("--") || param.starts_with("-") {
                true => Err(String::from(format!("The argument {0} must specify a parameter", key))),
                false => Ok(String::from(param))
            }
        }
    }
    Err(String::from(format!("No key found by the name {0}", key)))
}

fn parse_arguments(args: &Vec<String>) -> Result<Params, String> {
    let debug = args.contains(&String::from("-d"));
    let cwd = match env::current_dir() {
        Ok(p) => p,
        Err(e) => {
            return Err(String::from(format!("Error getting current directory: {}", e)));
        }
    };

    let mut missing_params: Vec<&str> = vec!();
    let path = get_key_value("-p", args.as_ref());
    let file = get_key_value("-f", args.as_ref());

    let bin = get_key_value("-b", args.as_ref());
    if bin.is_err() {
        missing_params.push("-b");
    }

    let mut src = String::new();
    if path.is_ok() && file.is_ok() {
        return Err(String::from("Only either -f or -p can be used at once."));
    }

    if path.is_ok() ^ file.is_ok() {
        src = path.or(file)?;
    }
    else {
        missing_params.push("[-p or -f]");
    }

    if !missing_params.is_empty() {
        return Err(String::from(format!("Missing required param(s): {0}", missing_params.join(", "))));
    }

    Ok(Params {
        src: std::path::PathBuf::new().join(cwd.clone()).join(src),
        bin_dir: std::path::PathBuf::new().join(cwd.clone()).join(bin?),
        debug,
    })
}

fn collect_shaders(params: &Params) -> Vec<std::path::PathBuf> {
    let mut result: Vec<std::path::PathBuf> = vec!();
    for entry in glob::glob(format!("{0}/**/*.glsl", params.src.to_str().unwrap()).as_str()).expect("Failed to read glob pattern") {
        if let Ok(path) = entry {
            // Ignore include types
            let file_name = String::from(path.file_name().unwrap().to_str().unwrap());
            if !file_name.contains("inc") {
                result.push(path);
            }
        }
    }
    result
}

fn compile_shaders(params: &Params, shaders: &Vec<std::path::PathBuf>) -> i32 {
    let mut compiled = 0;
    for shader in shaders {
        let mut cmd = std::process::Command::new("glslangValidator");
        cmd.arg("-o").arg(String::from(format!("{0}/{1}.spv", params.bin_dir.to_str().unwrap(), shader.file_stem().unwrap().to_str().unwrap())));
        cmd.arg("-V").arg(shader.to_str().unwrap());
        cmd.arg("--target-env").arg("vulkan1.4");
        if params.debug {
            cmd.arg("-g");
        }

        match cmd.output() {
            Ok(o) => {
                let msg = String::from_utf8_lossy(&o.stdout);
                if msg.contains("ERROR") {
                    println!("Failed to compile shader:\n{0}", msg);
                }
                else {
                    compiled += 1;
                }
            },
            Err(e) => {
                println!("Error: {0}", e);
            }
        }
    }

    compiled
}

fn main() {
    let args: Vec<String> = env::args().collect();

    let params_result = parse_arguments(args.as_ref());

    let has_help_flag = args.contains(&String::from("-h"));
    if params_result.is_err() || has_help_flag {
        println!("{0}{1}\
            \nUsage: nst [options]\
            \n\nOptions:\
            \n\t-h\t\tDisplay available options\
            \n\t-p [dir]\tShader source directory\
            \n\t-f [shader]\tSpecific shader source file\
            \n\t-b [dir]\tOutput directory\
            \n\t-d\t\tInclude debug information in compiled shaders",
            "[Nebula Shader Tools]".bright_purple().bold(),
            if params_result.is_err() && !has_help_flag { format!("\n\nError: {0}", params_result.err().unwrap()).bright_red() } else { "\n".normal() }
        );
        return;
    }

    let params = params_result.unwrap();
    // println!("Params:\n\tsrc: {0}\n\tbin: {1}\n\tdebug: {2}", params.src.display(), params.bin_dir.display(), params.debug);

    if !params.bin_dir.exists() {
        std::fs::create_dir(params.bin_dir.clone()).expect("Failed to create output directory.");
    }

    let shaders = collect_shaders(&params);
    let compiled_count = compile_shaders(&params, shaders.as_ref());

    println!("{0} {1} {2} {3}",
        "[nst]".bright_purple().bold(),
        "Compiled shaders:".bright_blue(),
        compiled_count.to_string().bold(),
        format!("(out of {0})", shaders.len()).to_string().italic());
}

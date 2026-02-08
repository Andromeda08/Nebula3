use std::collections::HashSet;
use std::env;
use colored::Colorize;
use glob;
use crate::util;

pub struct NSTParams {
    src:     std::path::PathBuf,
    bin_dir: std::path::PathBuf,
    debug:   bool,
}

fn parse_arguments(args: &Vec<String>) -> Result<NSTParams, String> {
    let debug = args.contains(&String::from("-d"));
    let cwd = match env::current_dir() {
        Ok(p) => p,
        Err(e) => {
            return Err(String::from(format!("Error getting current directory: {}", e)));
        }
    };

    let mut missing_params: Vec<&str> = vec!();
    let path = util::get_key_value("-p", args.as_ref());
    let file = util::get_key_value("-f", args.as_ref());

    let bin = util::get_key_value("-b", args.as_ref());
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

    Ok(NSTParams {
        src: std::path::PathBuf::new().join(cwd.clone()).join(src),
        bin_dir: std::path::PathBuf::new().join(cwd.clone()).join(bin?),
        debug,
    })
}

fn collect_shaders(params: &NSTParams) -> Vec<std::path::PathBuf> {
    // let excluded_extensions = HashSet::from([]);
    let mut result: Vec<std::path::PathBuf> = vec!();
    for entry in glob::glob(format!("{0}/**/*.glsl", params.src.to_str().unwrap()).as_str()).expect("Failed to read glob pattern") {
        if let Ok(path) = entry {
            // Ignore include types
            // let file_name = String::from(path.file_name().unwrap().to_str().unwrap());
            result.push(path);
        }
    }
    for entry in glob::glob(format!("{0}/**/*.hlsl", params.src.to_str().unwrap()).as_str()).expect("Failed to read glob pattern") {
        if let Ok(path) = entry {
            result.push(path);
        }
    }
    result
}

fn compile_shaders(params: &NSTParams, shaders: &Vec<std::path::PathBuf>) -> i32 {
    let mut compiled = 0;
    for shader in shaders {
        let mut cmd = std::process::Command::new("glslangValidator");
        cmd.arg("-e").arg("main");
        cmd.arg("-o").arg(format!("{0}/{1}.spv", params.bin_dir.to_str().unwrap(), shader.file_stem().unwrap().to_str().unwrap()));
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

pub fn nst(args: &Vec<String>) -> Result<(), String> {
    if args.contains(&String::from("-h")) {
        println!("{0}\
            \nUsage: nrt build-shaders [options]\
            \n\nOptions:\
            \n\t-h\t\tDisplay available options\
            \n\t-p [dir]\tShader source directory\
            \n\t-f [shader]\tSpecific shader source file\
            \n\t-b [dir]\tOutput directory\
            \n\t-d\t\tInclude debug information in compiled shaders",
            "[Nebula Resource Tools]".bright_purple().bold());
        return Ok(());
    }

    let params = match parse_arguments(args.as_ref()) {
        Ok(x) => x,
        Err(e) => return Err(e),
    };

    if !params.bin_dir.exists() {
        match std::fs::create_dir(params.bin_dir.clone()) {
            Err(_e) => return Err("Failed to create output directory.".to_string()),
            _ => {}
        }
    }

    let shaders = collect_shaders(&params);
    let compiled_count = compile_shaders(&params, shaders.as_ref());

    println!("{0} {1} {2} {3}",
        "[nst]".bright_purple().bold(),
        "Compiled shaders:".bright_blue(),
        compiled_count.to_string().bold(),
        format!("(out of {0})", shaders.len()).to_string().italic());

    Ok(())
}

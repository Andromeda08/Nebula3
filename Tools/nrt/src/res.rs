use std::env;
use std::path::PathBuf;
use colored::Colorize;
use crate::util;

pub struct CRParams {
    bin:      PathBuf,
    res:      PathBuf,
    fonts:    bool,
    textures: bool,
}

fn parse_arguments(args: &Vec<String>) -> Result<CRParams, String> {
    let cwd = match env::current_dir() {
        Ok(p) => p,
        Err(e) => {
            return Err(String::from(format!("Error getting current directory: {}", e)));
        }
    };

    let mut missing_params: Vec<&str> = vec!();
    let bin = util::get_key_value("-b", args.as_ref());
    if bin.is_err() {
        missing_params.push("-b");
    }

    let res = util::get_key_value("-r", args.as_ref());
    if res.is_err() {
        missing_params.push("-r");
    }

    if !missing_params.is_empty() {
        return Err(String::from(format!("Missing required param(s): {0}", missing_params.join(", "))));
    }

    Ok(CRParams {
        bin: PathBuf::new().join(cwd.clone()).join(bin?).canonicalize().unwrap(),
        res: PathBuf::new().join(cwd.clone()).join(res?).canonicalize().unwrap(),
        fonts: args.contains(&String::from("-f")),
        textures: args.contains(&String::from("-t")),
    })
}

pub fn copy_resource_dir(src: PathBuf, dst: PathBuf) -> Result<(), String> {
    let src_str = src.to_str().unwrap().to_string();
    let dst_str = dst.to_str().unwrap().to_string();

    let src_ok = src_str.contains("Nebula3") && src_str.contains("Resources");
    let dst_ok = dst_str.contains("Nebula3") && dst_str.contains("Resources")
            && (dst_str.contains("cmake-build-debug") || dst_str.contains("cmake-build-release"));

    if !src_ok || !dst_ok {
        return Err(format!("Check directories!\n- src: {:?}\n- dst: {:?}", src, dst).to_string());
    }

    if !src.exists() {
        return Err(format!("The specified directory ({:?}) does not exist.", src).to_string());
    }

    if dst.exists() {
        match std::fs::remove_dir_all(dst.clone()) {
            Err(e) => return Err(format!("Failed to delete directory: {:?} ({})", dst, e).to_string()),
            _ => {}
        }
    }

    match copy_dir::copy_dir(src.clone(), dst.clone()) {
        Err(e) => return Err(format!("Failed to copy directory: {:?} to {:?} ({})", src, dst, e).to_string()),
        _ => {}
    }

    Ok(())
}

pub fn copy_resources(args: &Vec<String>) -> Result<(), String> {
    if args.contains(&String::from("-h")) {
        println!("{0}\
            \n\nUsage: nrt copy-resources [options]\
            \n\nOptions:\
            \n\t-h\t\tDisplay help\
            \n\t-f\t\tCopy Fonts\
            \n\t-t\t\tCopy Textures\
            \n\t-b [dir]\tBinary directory ({1})\
            \n\t-r [dir]\tResources directory ({1})",
            "[Nebula Resource Tools]".bright_purple().bold(),
            "required".bright_red().italic());
        return Ok(());
    }

    let params = match parse_arguments(args.as_ref()) {
        Ok(x) => x,
        Err(e) => return Err(e),
    };

    let mut copy_msgs: Vec<String> = vec!();

    if params.fonts {
        match copy_resource_dir(
            params.res.clone().join("Fonts"),
            params.bin.clone().join("Fonts")
        ) {
            Err(e) => return Err(format!("(while copying fonts) {}", e).to_string()),
            _ => copy_msgs.push(format!("- {}", "fonts".bright_blue().bold())),
        }
    }
    if params.textures {
        match copy_resource_dir(
            params.res.clone().join("Textures"),
            params.bin.clone().join("Textures")
        ) {
            Err(e) => return Err(format!("(while copying textures) {}", e).to_string()),
            _ => copy_msgs.push(format!("- {}", "textures".bright_blue().bold())),
        }
    }

    if copy_msgs.is_empty() {
        println!("{} Warning: No resources were copied.", "[nrt]".bright_yellow().bold());
    }
    else {
        println!("{} Copied resource(s):\n\t{}", "[nrt]".bright_purple().bold(), copy_msgs.join("\n\t"));
    }

    Ok(())
}

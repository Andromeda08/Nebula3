use colored::Colorize;

pub fn get_main_help_string() -> String {
    format!("{0}\
            \n\nValid usage: nrt [command] [options]\
            \n\nAvailable commands ({1}):\
            \n\tbuild-shaders\tCompile shaders\
            \n\tcopy-resources\tCopy resources to binary directory",
            "[Nebula Resource Tools]".bright_purple().bold(),
            "for more info run a command with the flag -h".italic()
    ).to_string()
}

pub fn get_key_value(key: &str, args: &Vec<String>) -> Result<String, String> {
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

pub fn get_command(valid_commands: &Vec<String>, args: &Vec<String>) -> Result<String, ()> {
    if args.len() < 2 || !valid_commands.contains(&args[1]) {
        return Err(());
    }
    Ok(args[1].to_string())
}
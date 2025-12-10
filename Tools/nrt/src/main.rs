mod res;
mod nst;
mod util;

use std::env;
use colored::Colorize;

fn main() {
    let args: Vec<String> = env::args().collect();

    let valid_commands: Vec<String> = vec!("build-shaders".to_string(), "copy-resources".to_string());
    let cmd = match util::get_command(&valid_commands, &args) {
        Ok(x) => x,
        Err(_e) => {
            println!("{}", util::get_main_help_string());
            return
        }
    };

    match cmd.as_str() {
        "build-shaders" => {
            match nst::nst(&args) {
                Err(e) => println!("{}", format!("{0} Error: {1}", "[nrt]".bright_red().bold(), e)),
                _ => {},
            }
        },
        "copy-resources" => {
            match res::copy_resources(&args) {
                Err(e) => println!("{}", format!("{0} Error: {1}", "[nrt]".bright_red().bold(), e)),
                _ => {},
            }
        },
        _ => {}
    }
}

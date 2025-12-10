mod resources;
mod nst;
mod util;

use std::env;

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
                Err(e) => println!("[nrt] Error: {0}", e),
                _ => {},
            }
        },
        "copy-resources" => {
            println!("[nrt] The command \"copy-resources\" is not implemented yet");
        },
        _ => {}
    }
}

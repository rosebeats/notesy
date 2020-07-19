use std::io::prelude::*;
use std::{
    fs::File,
    os::unix::io::FromRawFd,
};

#[no_mangle]
pub extern "C" fn test_thread(fd_pipe: [i32; 2]) {
    let mut f = unsafe { File::from_raw_fd(fd_pipe[0]) };
    let mut received = [0; 1];
    
    let readres = f.read(&mut received).unwrap();
    if readres != 0 {
        println!("{}", received[0]);
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}

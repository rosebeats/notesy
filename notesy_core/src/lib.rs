use std::ffi::c_void;
use std::{ptr};

#[no_mangle]
pub extern "C" fn test_thread(context_ptr: *mut c_void) -> *mut c_void {
    // get context from main thread and set up a connection
    let ctx = unsafe{ zmq::Context::from_raw(context_ptr) };
    let main_socket = ctx.socket(zmq::PAIR).unwrap();
    main_socket.connect("inproc://test").unwrap();
    
    loop {
        let message = main_socket.recv_string(0).unwrap().unwrap();
        println!("{}", message);
        if message.eq("stop") {
            break;
        }
    }
    ptr::null_mut()
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}

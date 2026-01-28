# SSFHS - Simple Static File HTTP Server

Welcome to **SSFHS**, a fun and simple static file HTTP server! Why build it? Well... why not?!

---

## Features

- Supports **all HTTP methods**  
- Logs request date & IP for incoming requests 
- Logs request User-Agent
- Blocks pesky path traversal attacks (no sneaky `..` allowed!) 
- Dynamic content support (planned)
- Protected file list - keep your secrets safe 
- Custom error pages: 400, 403, 404 with style
- Custom index page support
- Dynamic file listing for directories 
- CLI arguments for easy config 
- Config file path customization  
- Custom server data directory path  
- Dedicated log file path  
- Configurable listen port  

Lots more features are brewing, stay tuned!  

---

## Building SSFHS

Just run:  
```bash
make
```

This compiles the server from source and puts the executable in the `build/` folder as `ssfhs`.

You can choose build modes too:  
- `RELEASE` (default, optimized)  
- `DEBUG` (includes debug info)  
- `NOASAN` (debug without address sanitizer)  

For example:  
```bash
make BUILD_MODE=DEBUG
```

---

## Examples & Usage

Want to see SSFHS in action? Check out the example setups!

- **Basic**: Contains a minimal test page, custom error pages, styling, and scripts that demonstrate malformed request handling and error scenarios.  
- **Minimal**: A barebones setup to get you started quickly.  
- **Dynamic**: Experimental support for dynamic content (still under development).  

---


## Configuration File

SSFHS uses a plain text config file to customize server behavior and features. Here's what you can set:

- **PROTECTED**: Paths to files that should never be served or shown in directory listings (e.g., config itself, readme, or secret files)  
- **400_PAGE, 403_PAGE, 404_PAGE**: Paths to your custom error pages to display for these HTTP status codes  
- **INDEX_PAGE**: Path to your custom index (homepage) file  
- **DYNAMIC**: List of files treated as dynamic, supporting special tags like `<ssfhs-dyn>` for runtime content generation (only in dynamic mode)  

Example config files are included in the [examples/basic](./examples/basic/ssfhs.conf) and [examples/dynamic](./examples/dynamic/ssfhs.conf) folders.  

---

Thanks for checking out SSFHS! Have fun serving files effortlessly :3

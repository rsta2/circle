let fs = require('fs');
let os = require('os');
let util = require('util');
let stdout = process.stdout;
let child_process = require('child_process');


// Command line options
let hexFile = null;
let serialPortName = null;
let serialPortOptions = {
    dataBits: 8,
    stopBits: 1,
    parity: 'none',
};
let flashBaud = 115200;
let userBaud = 115200;
let waitForAck = true;
let goSwitch = false;
let nogoSwitch = false;
let rebootMagic = null;
let rebootDelay = null;
let monitor = false;
let noFast = false;
let usingHC06 = false;
let noRespawn = false;
let goDelay = 0;

// The currently open serial port
let port;

// The serial port module (delayed load)
let SerialPort;

// Resolved runtime settings
let fastMode = false;
let willSendGoCommand;

// Open the serial port at specified baud rate, closing and reopening
// if currently open at a different rate
async function openSerialPortAsync(baudRate)
{
    // Already open?
    if (port != null)
    {
        // Correct baud rate already?
        if (serialPortOptions.baudRate == baudRate)
            return;

        // Close the port
        await closeSerialPortAsync();
    }
    
    // Load the serial port module and display handy message if can't
    if (!SerialPort)
    {
        try
        {
            SerialPort = require('serialport');
        }
        catch (err)
        {
            console.log(`\nCan't find module 'serialport'.`);
            console.log(`Please run 'npm install' in the flashy script folder:\n`);
            console.log(`   ` + __dirname + `$ npm install`);
            process.exit(7);
        }
    }
    
    // Remap WSL serial port names to Windows equivalent if appropriate
    if (os.platform() == 'win32' && serialPortName.startsWith(`/dev/ttyS`))
    {
        // This is a hacky fix for when launched from within a WSL session (possibly
        // related to Windows 11) where the SerialPort module fails with overlapped
        // I/O errors.  No sure why this happens but only seems to occur in the process
        // launched immediately from WSL.
        // As a work around, we simply respawn ourself with the same arguments + an
        // additional "--no-respawn" flag so we know not to do this again.
        if (!noRespawn)
        {
            // Use same args, but add the --no-respawn flag
            let args = process.argv.slice(1);
            args.push("--no-respawn");

            // Respawn
            let r = child_process.spawnSync(
                process.argv[0], 
                args,
                { 
                    stdio: 'inherit', 
                    shell: false
                }
            );
            
            // Quit with exit code of child process
            process.exit(r.status);
        }
        

        let remapped = `COM` + serialPortName.substr(9);
        stdout.write(`Using '${remapped}' instead of WSL port '${serialPortName}'.\n`)
        serialPortName = remapped;
    }

    // Configure options
    serialPortOptions.baudRate = baudRate;

    // Open it
    stdout.write(`Opening ${serialPortName} at ${baudRate}...`)
    port = new SerialPort(serialPortName, serialPortOptions, function(err) {
        if (err)
        {
            fail(`Failed to open serial port: ${err.message}`);
        }
    });
    stdout.write(`ok\n`);
}

function discardOldLines(str)
{
    let nlpos = str.lastIndexOf('\n');
    if (str >= 0)
        return str.substr(nlpos)
    else
        return str;
}

async function drainSerialPortAsync()
{
    // Drain port
    await new Promise((resolve, reject) => {
        port.drain((function(err) {
            if (err)
                reject(err);
            else
                resolve();
        }));
    });
}

// Close the serial port (if it's open)
async function closeSerialPortAsync()
{
    if (port)
    {
        stdout.write(`Closing serial port...`)

        // Drain
        await drainSerialPortAsync();

        // Close the port
        await new Promise((resolve, reject) => {
            port.close(function(err) { 
                if (err)
                    reject(err);
                else
                {
                    resolve();
                }
            });
        });

        stdout.write(`ok\n`);
    }
}

// Async wrapper for write operations so we don't have to deal with callbacks
async function writeSerialPortAsync(data) 
{
    return new Promise((resolve,reject) => 
    {
        port.write(data, function(err) 
        {
            if (err)
                reject(err);
            else
                resolve();
        });
    });
};

function bootloaderErrorWatcher(data)
{
    let str = data.toString(`utf8`);

    let err = (/#ERR:(.*)\r/gm).exec(str);
    if (err)
    {
        console.error(`\n\nAn error occurred during the transfer: ${err[1]}`);
        process.exit(9);
    }
}

function watchForBootloaderErrors(enable)
{
    // Redundant?
    if (port.listenerEnabled == enable)
        return;
    port.listenerEnabled = enable;
    
    // Install/remove listener
    if (enable)
    {
        port.on('data', bootloaderErrorWatcher);
    }
    else
    {
        port.removeListener('data', bootloaderErrorWatcher);
    }
}



// Receives IHEX formatted ascii text and converts to binary
// stream as understood by the bootloader.
function binary_encoder()
{
    let state = 0;
    let buffer = Buffer.alloc(65536);
    let buffer_used = 0;
    let unsent_nibble = -1;
    let record_bytes_left = -1;

    // Flush the fast flash buffer
    async function flush()
    {
        if (buffer_used)
        {
            await writeSerialPortAsync(buffer.subarray(0, buffer_used));
            buffer_used = 0;
        }
    }

    // Write a byte
    async function write(inbyte)
    {
        if (state == 0)
        {
            if (inbyte == 0x3a)     // ":"
            {
                // Flush buffer?
                // When using an HC-06 module as a serial device,
                // flush on every record to fix transfer failure.
                if (buffer_used > buffer.length - 1024 || usingHC06)
                    await flush();

                // start of new record
                buffer[buffer_used++] = 0x3d;     // "="
                fast_record_length = -1;
                state = 1;
                record_bytes_left = -1;
                return;
            }
            else
            {
                // Must be white space
                if (inbyte != 0x20 && inbyte != 0x0A && inbyte != 0x0D)
                {
                    fail("Invalid .hex file, unexpected character outside record");
                }
                return;
            }
        }

        // Convert the incoming character to a hex nibble
        let nibble;
        if (inbyte >= 0x30 && inbyte <= 0x39)
            nibble = inbyte - 0x30;
        else if (inbyte >= 0x41 && inbyte <= 0x46)
            nibble = inbyte - 0x41 + 0xA;
        else if (inbyte >= 0x61 && inbyte <= 0x66)
            nibble = inbyte - 0x61 + 0xA;
        else
            fail("Invalid .hex file, expected hex digit");

        if (state == 1)
        {
            // First hex nibble, store it
            state = 2;
            unsent_nibble = nibble;
            return;
        }

        if (state == 2)
        {
            // Second hex nibble, calculate full byte
            let byte = ((unsent_nibble << 4) | nibble);

            // Write it
            buffer[buffer_used++] = byte;

            // Initialize the record length
            if (record_bytes_left == -1)
                record_bytes_left = byte + 5;  

            // Update record length and check for end of record
            record_bytes_left--;
            if (record_bytes_left == 0)
                state = 0;
            else
                state = 1;
        }
    }

    return { flush, write };
}

// Send the reboot magic string
async function sendRebootMagic()
{   
    // Open serial port
    await openSerialPortAsync(userBaud);

    // Send it
    stdout.write(`Sending reboot magic '${rebootMagic}'...`)
    await writeSerialPortAsync(rebootMagic);
    stdout.write(`ok\n`);

    // Delay
    if (rebootDelay)
    {
        stdout.write(`Delaying for ${rebootDelay}ms while rebooting...`);
        await delay(rebootDelay);
        stdout.write(`ok\n`);
    }
}

// Flash the device with the hex file
async function flashDevice()
{   
    // Open serial port
    await openSerialPortAsync(flashBaud);

    // Reset signal consists of 256 x 0x80 chars, followed
    // by a reset 'R' command.  The idea here is the 0x80s will
    // flush a previously canceled flash out of a binary
    // record state.  0x80 is used in case the bootloader is
    // cancelled at the start of a binary record and we don't 
    // want to write a lo-memory address that will trash the
    // bootloader itself.
    let resetBuf = Buffer.alloc(257, 0x80);
    resetBuf[256] = 'R'.charCodeAt(0);

    // Wait for `IHEX` from device as ack it's ready
    if (waitForAck)
    {
        // Send a reset command 
        // (requires the newest version of the booloader kernal)
        stdout.write(`Sending reset command...`);

        // Setup receive listener
        let resolveDeviceReady;
        let buf = "";
        port.on('data', function(data) {

            buf += data.toString(`utf8`);
            if (buf.includes(`IHEX`))
            {
                stdout.write(`ok\n`);

                if (!noFast)
                {
                    // If the device responds with IHEX-F it's got
                    // the fast bootloader so switch to that mode unless
                    // disabled by command line switch
                    if (buf.includes(`IHEX-F`))
                    {
                        stdout.write("Fast mode enabled\n");
                        fastMode = true;
                    }
                }

                if (resolveDeviceReady)
                    resolveDeviceReady();
            }
            buf = discardOldLines(buf);

        });

        // Send reset command
        await writeSerialPortAsync(resetBuf);

        // Set the reset
        stdout.write(`ok\n`);
        stdout.write(`Waiting for device...`);

        // Wait for it
        await new Promise((resolve, reject) => {
            resolveDeviceReady = resolve;
        });
        port.removeAllListeners('data');

        // Now that we know the device is in a good state, watch for errors
        watchForBootloaderErrors(true);
    }
    else
    {
        // Send reset command
        await writeSerialPortAsync(resetBuf);
    }

    // Make sure fast mode is enabled when using an HC-06
    if(!fastMode && usingHC06)
    {
        fail(`Bootloader doesn't support fast mode while HC-06 depends on it`);
    }

    // Create fast write binary encoder
    let binenc = fastMode ? binary_encoder() : null;

    // Copy to device
    let startTime = new Date().getTime();
    stdout.write(`Sending`);
    let fd = fs.openSync(hexFile, `r`);    
    let buf = Buffer.alloc(4096);
    while (true)
    {
        // Read from hex file
        let bytesRead = fs.readSync(fd, buf, 0, buf.length);
        if (bytesRead == 0)
            break;

        if (fastMode)
        {
            // In fast mode, push each byte through the binary encoder
            for (let i=0; i<bytesRead; i++)
            {
                await binenc.write(buf[i]);
            }
        }
        else
        {
            // Write directly to serial port
            await writeSerialPortAsync(buf.subarray(0, bytesRead));
        }
        stdout.write(`.`);
    }
    fs.closeSync(fd);

    // Flush the fast flash buffer
    if (fastMode)
    {
        binenc.flush();
    }

    // Wait for any pending errors
    // (If we're about to send the go command, it will pick up errors
    // before it's acked so don't need to wait here)
    if (waitForAck && !willSendGoCommand)
    {
        // Wait for everything to be sent
        await drainSerialPortAsync();

        // Small delay in case there's a pending error coming from the bootloader
        await delay(10);
        port.removeAllListeners('data');
    }

    // Done
    stdout.write(`ok\n`);

    // Log time taken
    let elapsedTime = new Date().getTime() - startTime;
    stdout.write(`Finished in ${((elapsedTime / 1000).toFixed(1))} seconds.\n`);
}


// Send the go command and wait for ack
async function sendGoCommand()
{   
    // Open serial port
    await openSerialPortAsync(flashBaud);

    // Send it
    stdout.write(`Sending go command...`)

    // Set a start delay
    if (goDelay)
    {
        await writeSerialPortAsync(`s${(goDelay*1000).toString(16).toUpperCase()}\n`);
    }

    // Wait until we receive `--` indicating device received the go command
    if (waitForAck)
    {
        // Setup receive listener
        let resolveAck;
        let buf = "";
        port.on('data', function(data) {

            buf += data.toString(`utf8`);
            if (buf.includes(`\r--\r\n\n`))
            {
                stdout.write(`ok\n`);
                resolveAck();
            }
            buf = discardOldLines(buf);


        });

        // Enable error watch
        watchForBootloaderErrors(true);

        // Send command
        await writeSerialPortAsync('g');

        // Wait for it
        await new Promise((resolve, reject) => {
            resolveAck = resolve;
        });
        port.removeAllListeners('data');
    }
    else
    {
        await writeSerialPortAsync('g');
        stdout.write(`ok\n`);
    }
}

// Start serial monitor
async function startMonitor()
{   
    // Open serial port
    await openSerialPortAsync(userBaud);

    // Bootloader shouldn't be running so remove error watcher
    watchForBootloaderErrors(false);

    stdout.write("Monitoring....\n");

    // Setup receive listener
    let resolveDeviceReady;
    port.removeAllListeners('data');
    port.on('data', function(data) {

        var str = data.toString(`utf8`);
        stdout.write(str);
    });

    // Wait for the never delivered promise to keep alive
    await new Promise((resolve) => { });
}

// Async delay helper
async function delay(period)
{
    return new Promise((resolve) => {
        setTimeout(resolve, period);
    })
}

// Help!
function showHelp()
{
    console.log(`Usage: node flashy <serialport> [<hexfile>] [options]`);
    console.log(`All-In-One Reboot, Flash and Monitor Tool`);
    console.log(``);
    console.log(`<serialport>       Serial port to write to`);
    console.log(`<hexfile>          The .hex file to write (optional)`);
    console.log(`--flashbaud:<N>    Baud rate for flashing (default=115200)`);
    console.log(`--userbaud:<N>     Baud rate for monitor and reboot magic (default=115200)`);
    console.log(`--noack            Send without checking if device is ready`);
    console.log(`--fast             Force fast mode flash`);
    console.log(`--nofast           Disable fast mode`);
    console.log(`--nogo             Don't send the go command after flashing`);
    console.log(`--go               Send the go command, even if not flashing`);
    console.log(`--godelay:<ms>     Sets a delay period for the go command`);
    console.log(`--reboot:<magic>   Sends a magic reboot string at user baud before flashing`);
    console.log(`--rebootdelay:<ms> Delay after sending reboot magic`);
    console.log(`--monitor          Monitor serial port`);
    console.log(`--hc06             Hints that a HC-06 is used as a serial device (cannot be used with --nofast)`)
    console.log(`--help             Show this help`);
}

// Abort with message
function fail(msg)
{
    console.error(msg);
    console.error(`Run with --help for instructions`);
    process.exit(7);
}

// Parse command line args
function parseCommandLine()
{
    for (let i=2; i<process.argv.length; i++)
    {   
        let arg = process.argv[i];
        if (arg.startsWith(`--`))
        {
            let parts = arg.substr(2).split(':');
            let sw = parts[0];
            let value = parts[1];
            switch (sw.toLowerCase())
            {
                case `flashbaud`:
                    flashBaud = Number(value);
                    break;

                case `noack`:
                    waitForAck = false;
                    break;

                case `help`:
                    showHelp();
                    process.exit(0);

                case `nogo`:
                    nogoSwitch = true;
                    break;

                case `go`:
                    goSwitch = true;
                    break;

                case `godelay`:
                    goDelay = Number(value);
                    break;

                case `reboot`:
                    rebootMagic = value;
                    break;

                case `rebootdelay`:
                    rebootDelay = Number(value);
                    break;

                case `monitor`:
                    monitor = true;
                    break;

                case `userbaud`:
                    userBaud = Number(value);
                    break;

                case `fast`:
                    fastMode = true;
                    break;

                case `nofast`:
                    noFast = true;
                    break;

                case `hc06`:
                    usingHC06 = true;
                    break;

                case "no-respawn":
                    noRespawn = true;
                    break;

                default:
                    fail(`Unknown switch --${sw}`);
            }
        }
        else
        {
            // First arg is serial port name
            if (serialPortName == null)
            {
                serialPortName = arg;
                continue;
            }
            else if (hexFile == null)
            {
                // Second arg is the .hex file
                hexFile = arg;
                
                // Sanity check
                if (!arg.toLowerCase().endsWith('.hex'))
                {
                    console.error(`Warning: hex file '${arg}' doesn't have .hex extension.`);
                }
            }
            else
            {
                fail(`Too many command line args: '${arg}'`);
            }
        }
    }

    // Can't do anything without a serial port
    if (!serialPortName)
        fail(`No serial port specified`);

    // HC-06 module must use fast mode (binary encoder) in order to work
    if(usingHC06 && noFast)
    {
        fail(`Cannot use --hc06 and --nofast`);
    }
}


// Run async
(async function()
{
    // parse the command line
    parseCommandLine();

    willSendGoCommand = (hexFile && !nogoSwitch) || (!hexFile && goSwitch);

    // Reboot
    if (rebootMagic)
        await sendRebootMagic();

    // Flash
    if (hexFile)
        await flashDevice();

    // Go
    if (willSendGoCommand)
        await sendGoCommand();

    // Monitor
    if (monitor)
        await startMonitor();

    // Finished
    await closeSerialPortAsync();
    stdout.write(`Done!\n`);
    process.exit(0);
})();

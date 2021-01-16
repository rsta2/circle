let fs = require('fs');
let os = require('os');
let util = require('util');
let stdout = process.stdout;

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

// The currently open serial port
let port;

// The serial port module (delayed load)
let SerialPort;

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

// Close the serial port (if it's open)
async function closeSerialPortAsync()
{
    if (port)
    {
        stdout.write(`Closing serial port...`)

        // Close the port
        await new Promise((resolve, reject) => {
            port.close(function(err) { 
                if (err)
                    reject(err);
                else
                {
                    stdout.write(`ok\n`);
                    resolve();
                }
            });
        });
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

    // Open the hex file
    let fd = fs.openSync(hexFile, `r`);    
    let buf = Buffer.alloc(4096);
    let stat = fs.fstatSync(fd);

    // Wait for `IHEX` from device as ack it's ready
    if (waitForAck)
    {
        // Send a reset command 
        // (requires the newest version of the booloader kernal)
        stdout.write(`Sending reset command...`);
        await writeSerialPortAsync('R');
        stdout.write(`ok\n`);

        stdout.write(`Waiting for device...`);

        // Setup receive listener
        let resolveDeviceReady;
        port.on('data', function(data) {

            if (data.toString(`utf8`).includes(`IHEX`))
            {
                stdout.write(`ok\n`);
                if (resolveDeviceReady)
                    resolveDeviceReady();
            }

        });

        // Wait for it
        await new Promise((resolve, reject) => {
            resolveDeviceReady = resolve;
        });
        port.removeAllListeners('data');
    }

    // Copy to device
    stdout.write(`Sending ${stat.size} bytes`);
    while (true)
    {
        // Read from hex file
        let bytesRead = fs.readSync(fd, buf, 0, buf.length);
        if (bytesRead == 0)
            break;

        // Write to serial port
        await writeSerialPortAsync(buf.subarray(0, bytesRead));
        stdout.write(`.`);
    }

    // Done
    stdout.write(`ok\n`);
}


// Send the go command and wait for ack
async function sendGoCommand()
{   
    // Open serial port
    await openSerialPortAsync(flashBaud);

    // Send it
    stdout.write(`Sending go command...`)
    await writeSerialPortAsync('g');

    // Wait until we receive `--` indicating device received the go command
    if (waitForAck)
    {
        // Setup receive listener
        let resolveAck;
        port.on('data', function(data) {

            if (data.toString(`utf8`).includes(`\r--\r\n\n`))
            {
                stdout.write(`ok\n`);
                resolveAck();
            }

        });

        // Wait for it
        await new Promise((resolve, reject) => {
            resolveAck = resolve;
        });
        port.removeAllListeners('data');
    }
    else
    {
        stdout.write(`ok\n`);
    }
}

// Start serial monitor
async function startMonitor()
{   
    // Open serial port
    await openSerialPortAsync(userBaud);

    // Setup receive listener
    let resolveDeviceReady;
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
    console.log(`--nogo             Don't send the go command after flashing`);
    console.log(`--go               Send the go command, even if not flashing`);
    console.log(`--reboot:<magic>   Sends a magic reboot string at user baud before flashing`);
    console.log(`--rebootdelay:<ms> Delay after sending reboot magic (only needed with --noack)`);
    console.log(`--monitor          Monitor serial port`);
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
}


// Run async
(async function()
{
    // parse the command line
    parseCommandLine();

    // Reboot
    if (rebootMagic)
        await sendRebootMagic();

    // Flash
    if (hexFile)
        await flashDevice();

    // Go
    if ((hexFile && !nogoSwitch) || (!hexFile && goSwitch))
        await sendGoCommand();

    // Monitor
    if (monitor)
        await startMonitor();

    // Finished
    await closeSerialPortAsync();
    stdout.write(`Done!\n`);
})();
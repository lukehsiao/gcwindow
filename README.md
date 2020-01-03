# Eyelink 1000 Latency Measurements

## Dependencies

### EyeLink Developers Kit for Linux

The EyeLink Developers Kit for Linux packages include:

* EyeLink C API libraries
     * The EyeLink Programmers Guide can be found in /usr/share/EyeLink/
     * Sample C programs that use SDL as the graphics environment can be found
       at /usr/share/EyeLink/SampleExperiments/
* EyeLink EDF Access API for direct reading of .edf files in C
     * EDF Access C API user manual can be found at /usr/share/edfapi
     * Sample EDF API program can be found at
       /usr/share/edfapi/EDF_Access_API/Example
* EyeLink Pylink library for Python
     * These libraries are symlink'd into the dist-packages folder for native
       python installs found during installation
     * Pylink libraries, examples, and documentation can be found in
       /usr/share/EyeLink/pylink
* edf2asc, a command line tool for converting .edf files to ascii
* EDF Converter (i.e. Visual EDF2ASC), a GUI interface for converting .edf files
  to ascii

Steps to install:

1. Add signing key
   ```
   $ wget -O - "http://download.sr-support.com/software/dists/SRResearch/SRResearch_key" | sudo apt-key add -
   ```
2. Add apt repository
   ```
   $ sudo add-apt-repository "deb http://download.sr-support.com/software SRResearch main"
   $ sudo apt-get update
   ```
3. Install latest release of EyeLink Developers Kit for Linux
   ```
   $ sudo apt-get install eyelink-display-software
   ```
Alternatively, a tar of DEBs is available [at this link][debs].

### Network Configuration

The Display PC and the Host PC communicate via a direct Ethernet connection
between the two computers. In order for Experiment Builder and other third party
programming packages/languages to use this connection, you must use a static IP
address for the Ethernet port that is used to connect to the Host PC. This
static IP should be:

    IP Address: 100.1.1.2
    Subnet Mask: 255.255.255.0

See: [SR Research Support: Getting Started with Display PC][display]

[display]: https://www.sr-support.com/forum/how-tos/getting-started/getting-started-with-eyelink-system-installation-and-usage/52251-getting-started-with-display-pc-configuration
[debs]: http://download.sr-support.com/linuxDisplaySoftwareRelease/eyelink-display-software_1.11_x64_debs.tar.gz

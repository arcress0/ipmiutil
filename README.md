# ipmiutil
ipmiutil is an easy to use set of IPMI server management utilities.  It can get/set sensor readings &amp; thresholds, automate SEL management, do SOL console, etc.  Supports Linux, Windows, BSD, Solaris, MacOSX.  The only IPMI project tool that runs natively on Windows.  See http://ipmiutil.sf.net for rpms, etc. (formerly called panicsel).  It can run driverless in Linux for use on boot media or embedded environments.

Features
- Handles IPMI sensors, events, remote console, power control, etc.
- Native builds on many OSs: Linux, Windows, BSD, Solaris, MacOSX, HPUX
- The only IPMI project natively supporting Windows drivers
- IPMIUTIL supports any IPMI-compliant vendor firmware
- Detects and handles OEM-specific IPMI firmware variants
- IPMIUTIL interprets various vendor OEM-specific sensor values automatically.
- Any IPMI values not yet recognized at least return the values, rather than just 'na' or Unknown.
- Shared library for custom applications, sample source included
- BSD license is compatible with open-source or commercial use
- Linux driverless support is ideal for boot media or embedded

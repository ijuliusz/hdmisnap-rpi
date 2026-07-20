\## Quick Start (using the precompiled binary)



No need to compile anything — a precompiled binary is included in this repo.



If `git clone` doesn't work on your Pi (e.g. DNS issues), you can

download the files directly with `curl`:



\\`\\`\\`bash

sudo mkdir hdmisnap

cd hdmisnap

sudo curl --resolve raw.githubusercontent.com:443:185.199.108.133 -o hdmisnap "https://raw.githubusercontent.com/ijuliusz/hdmisnap-rpi/main/hdmisnap"

sudo curl --resolve raw.githubusercontent.com:443:185.199.108.133 -o hdmisnap.bin "https://raw.githubusercontent.com/ijuliusz/hdmisnap-rpi/main/hdmisnap.bin"

sudo chmod +x hdmisnap hdmisnap.bin

sudo ./hdmisnap test.bmp

\\`\\`\\`



Or, if `git` works fine on your system:



\\`\\`\\`bash

git clone https://github.com/ijuliusz/hdmisnap-rpi.git

cd hdmisnap-rpi

chmod +x hdmisnap hdmisnap.bin

sudo ./hdmisnap output.bmp

\\`\\`\\`



If no filename is given, it defaults to `screen.bmp` in the current directory.



> \*\*Note:\*\* the binary was built on Raspberry Pi OS wheezy (armhf). It

> should run on later releases thanks to glibc backward compatibility,

> but has not been verified on bookworm/trixie — if it doesn't run,

> build from source instead (see below).



> \*\*Note on the `--resolve` flag:\*\* this pins `raw.githubusercontent.com`

> to a specific IP, which helps if your Pi has trouble resolving GitHub's

> hostnames via DNS. If you don't have that problem, a plain `curl -O`

> or `wget` will work just as well.


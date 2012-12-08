How to make it work!

./_autosetup
export BOINCDIR=/path/to/source/boinc-rpi/boinc/boinc_7.1.0
./configure --disable-server
make
sudo -u boinc mkdir -p /var/lib/boinc-client/projects/setiathome.berkeley.edu
sudo -u boinc cp app_info.xml /var/lib/boinc-client/projects/setiathome.berkeley.edu
sudo -u boinc cp client/setiathome-7.0.armv6l-unknown-linux-gnu /var/lib/boinc-client/projects/setiathome.berkeley.edu/setiathome_enhanced
sudo -u boinc boinccmd --project_attach http://setiathome.berkeley.edu <your_key>

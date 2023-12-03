#! /bin/bash
# Update apt-get
sudo apt-get update

# Install visual tools
sudo apt-get install -y gtkterm geany geany-plugins git-cola meld

# Get geany color scheme(s)
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/delt-dark.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/dark-colors.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/himbeere.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/mc.conf

# Install Visual Studio Code
sudo apt update
sudo apt install dirmngr ca-certificates software-properties-common apt-transport-https curl -y
curl -fSsL https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor | sudo tee /usr/share/keyrings/vscode.gpg >/dev/null
echo deb [arch=amd64 signed-by=/usr/share/keyrings/vscode.gpg] https://packages.microsoft.com/repos/vscode stable main | sudo tee /etc/apt/sources.list.d/vscode.list
sudo apt install -y code

#! /bin/bash
# Setup user name and email in git
printf "\n\n<<<Setup Git with the following user name and email address>>>\n"
read -p 'Username: ' user
read -p 'Email addr: ' email
git config --global user.name "${user}"
git config --global user.email "${email}"
printf "\n"

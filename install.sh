#!/bin/sh
mkdir -p ~/.local/bin/
mkdir -p ~/.local/user/quicklaunch/
cp normalize rotate ~/.local/bin/
cp normalize.desktop rotate.desktop ~/.local/user/quicklaunch/
xdg-open "https://extensions.gnome.org/extension/57/wwwdotorg-quick-launch/"

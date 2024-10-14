de()
{
    echo "$1" >> godot.desktop
}
echo '[Desktop Entry]' > godot.desktop
de 'Type=Application'
de 'Version=1.0'
de 'Name=Godot'
de 'Comment=Godot Engine Editor'
de "Path=$(realpath .)"
de 'Exec=bin/godot.x11.opt.tools.64'
de "Icon=$(realpath icon.svg)"
de 'Terminal=false'
de 'Categories=Development'

desktop-file-validate godot.desktop
cp godot.desktop ~/.local/share/applications
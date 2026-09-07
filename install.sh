# Compilation
gcc -O2 -o prompt-decorator bash-prompt-decorator.c

# Installation système
sudo cp prompt-decorator /usr/local/bin/

cat install-to-bash.sh >> ~/.bashrc
source ~/.bashrc


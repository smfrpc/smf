# -*- mode: ruby -*-
# vi: set ft=ruby :
VAGRANTFILE_API_VERSION = "2"

$provision_smf = <<SCRIPT
echo Provisioning SMF

export PYTHONUNBUFFERED=1
echo '/swapfile none swap defaults 0 0' | sudo tee -a /etc/fstab
sudo dnf upgrade -y
sudo dnf install -y git htop

cd /vagrant/meta

echo $(pwd)
. source_ansible_bash
nice -n 19 ansible-playbook playbooks/devbox_all.yml

SCRIPT

git_branch=`git rev-parse --abbrev-ref HEAD `
Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.hostname = "smurf" + "." + git_branch.gsub(/[\/,:()\s_]/ , '.')
  config.vm.box = "fedora/25-cloud-base"
  config.ssh.keep_alive = true
  # From https://fedoraproject.org/wiki/Vagrant, setup vagrant-hostmanager
  if Vagrant.has_plugin? "vagrant-hostmanager"
      config.hostmanager.enabled = true
      config.hostmanager.manage_host = true
  end
  config.vm.provision "shell", inline: $provision_smf
  config.vm.provider :virtualbox do |vb|
    vb.memory = 8096
    vb.cpus = 4
  end
end

# -*- mode: ruby -*-
# vi: set ft=ruby :
VAGRANTFILE_API_VERSION = "2"

$provision_smf = <<SCRIPT
echo Provisioning SMF

sudo dnf upgrade -y
sudo dnf install -y git

cd /vagrant/meta
source source_ansible_bash

echo $(pwd)
nice -n 19 ./tmp/bin/ansible-playbook playbooks/devbox_all.yml

SCRIPT


Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "fedora/25-cloud-base"
  if Vagrant.has_plugin? "vagrant-vbguest"
    config.vbguest.auto_update = true
  end
  # From https://fedoraproject.org/wiki/Vagrant, setup vagrant-hostmanager
  if Vagrant.has_plugin? "vagrant-hostmanager"
      config.hostmanager.enabled = true
      config.hostmanager.manage_host = true
  end
  config.disksize.size = '15GB'
  config.vm.provision "shell", inline: $provision_smf
  config.vm.provider :virtualbox do |vb|
    vb.memory = 8096
    vb.cpus = 4
  end
end

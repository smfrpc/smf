# -*- mode: ruby -*-
# vi: set ft=ruby :
VAGRANTFILE_API_VERSION = "2"

$provision_smf = <<SCRIPT
echo Provisioning SMF
cd /vagrant/meta
. source_ansible_bash
ansible-playbook playbooks/devbox_all.yml
SCRIPT


Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "fedora/25-cloud-base"
  config.vbguest.auto_update = true

  # From https://fedoraproject.org/wiki/Vagrant, setup vagrant-hostmanager
  if Vagrant.has_plugin?("vagrant-hostmanager")
      config.hostmanager.enabled = true
      config.hostmanager.manage_host = true
  end

  # Bring system up to date
  config.vm.provision "shell", inline: "sudo dnf upgrade -y"
  config.vm.provision "shell", inline: "sudo dnf -y install git"
  # From the smf intro
  config.vm.provision "shell", inline: $provision_smf

  # Use virtualbox provider
  config.vm.provider :virtualbox do |vb|
    vb.memory = 8096
    vb.cpus = 4
  end
end

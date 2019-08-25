#!/bin/bash
set -ex

# update system
apt-get update
apt-get install -y nginx
cp nginx/default /etc/nginx/sites-available/default
systemctl restart nginx

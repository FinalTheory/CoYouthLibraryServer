#!/usr/bin/python
# -*- coding: utf-8 -*-

from os import getcwd
from ConfigParser import ConfigParser

cfg_name = 'config.ini'
if "cgi-bin" in getcwd():
    cfg_path = getcwd() + "/"
else:
    cfg_path = getcwd() + "/www/cgi-bin/"

ConfigDict = {}

def LoadConfig():
    global ConfigDict
    config = ConfigParser()
    config.read(cfg_path + cfg_name)
    for sec in config.sections():
        for opt in config.options(sec):
            ConfigDict[opt] = config.get(sec, opt)


def ModifyConfig():
    pass

if __name__ == "Config":
    LoadConfig()

if __name__ == "__main__":
    pass
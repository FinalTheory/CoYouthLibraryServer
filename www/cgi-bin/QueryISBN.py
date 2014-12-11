#!/usr/bin/pythondevice = '/dev/video0'
# -*- coding: utf-8 -*-

import simplejson as json
from urllib2 import urlopen
from Config import ConfigDict

def QueryISBN( StrISBN ):
    json_dict = json.loads(urlopen(ConfigDict["book_query_url"] +
                                   urlopen(ConfigDict["isbn_query_url"] +
                                           StrISBN).geturl().rstrip('/').split('/')[-1]).read())
    json_dict["ISBN"] = StrISBN
    return json.dumps(json_dict)


if __name__ == "__main__":
    print QueryISBN("9787508321752")
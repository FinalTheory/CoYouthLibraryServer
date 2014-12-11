#!/usr/bin/python
# -*- coding: utf-8 -*-

import cgi
import sys
import sha
import time
import MySQLdb
from random import random
from zbar import Processor
from datetime import datetime
from Config import ConfigDict
from os import getcwd, environ
from Cookie import SimpleCookie
from mako.template import Template
from urllib2 import urlopen, unquote
import json


class LibraryManager():

    #*******************构造函数*******************

    def __init__(self):
        # 定义HTTP响应头
        self.HTML_Headers = "Content-type: text/html\r\n\r\n"

        # 登陆信息初始化
        self.UID = ""
        self.UserName = ""
        self.Auth = -1
        self.TimeLeft = 0
        self.CheckLoggedIn()

        # HTML模版位置初始化
        if "cgi-bin" in getcwd():
            self.Temp_Path = getcwd() + "/../html/templates/"
        else:
            self.Temp_Path = getcwd() + "/www/html/templates/"

        # 建立POST Data与对应函数的映射关系
        self.map = {
            "Index": self.IndexPage,
            "Login": self.Login,
            "LoginPage": self.LoginPage,
            "Logout": self.LogOut,
            "BibliographyManagementPage": self.BibliographyManagementPage,
            "ReadCode": self.ReadCode,
            "QueryISBN": self.QueryISBN,
            "ManageBibliography" : self.ManageBibliography,
            "ModifyBibliographyPage": self.ModifyBibliographyPage,
        }

        # 初始化CGI
        self.form = cgi.FieldStorage()
        action = self.form.getfirst('action', "Index")

        # 返回数据
        # 注意请求的处理过程中可能会影响到Header
        Result = self.map[action]()
        print self.HTML_Headers + Result

    #*******************页面输出*******************

    def CreateMyTemplate(self, filename):
        return Template(
            filename=self.Temp_Path + filename,
            input_encoding='utf-8',
            output_encoding="utf-8",
            encoding_errors='replace'
        )

    def RefreshPage(self, msg=u"操作成功", wait=1, url="/"):
        Text = """
        <html>
        <head>
        <title>Welcome!
        </title>
        </head>
        <body>
        %s
        <meta http-equiv="refresh" content="%d; url=%s">
        </body>
        </html>
        """ % (msg.encode("utf-8"), wait, url)
        return Text

    def IndexPage(self):
        MyTemplate = self.CreateMyTemplate("Index.html")
        return MyTemplate.render(
            SiteName=ConfigDict["sitename"].decode("utf-8"),
            UserName = self.UserName,
            Auth = self.Auth,
            CookieExpireSeconds = self.TimeLeft
        )

    def LoginPage(self):
        MyTemplate = self.CreateMyTemplate("Login.html")
        return MyTemplate.render()

    def BibliographyManagementPage(self):
        # 首先过滤权限不允许的情况
        if self.Auth == -1:
            return self.RefreshPage(u"请先登陆！")
        elif self.Auth == 1:
            return self.RefreshPage(u"普通用户无权操作书目！")
        elif self.Auth == 2:
            return self.RefreshPage(u"被封禁用户无权操作书目！")
        elif self.Auth == 0:
            MyTemplate = self.CreateMyTemplate("BibliographyManagement.html")
            return MyTemplate.render(
                SiteName=ConfigDict["sitename"].decode("utf-8"),
            )

    def ModifyBibliographyPage(self):
        if self.Auth == -1:
            return "<h2>请先登陆！</h2>"
        elif self.Auth == 1:
            return "<h2>普通用户无权操作书目！</h2>"
        elif self.Auth == 2:
            return "<h2>被封禁用户无权操作书目！</h2>"
        elif self.Auth == 0:
            ColName = self.form.getfirst("subaction")
            KeyWord = unquote(self.form.getfirst("KeyWord"))
            sql = 'SELECT ISBN, BookName, Author, Publisher, PubDate, TimeLimit, LastVis ' \
                  'FROM Bibliography WHERE %s like "%%%s%%"' % (ColName, KeyWord)
            conn = self.ConnectMySQL()
            cursor = conn.cursor()
            try:
                cursor.execute(sql)
                conn.commit()
                Results = cursor.fetchall()
                cursor.close()
                if Results:
                    MyTemplate = self.CreateMyTemplate("ModifyBibliography.html")
                    return MyTemplate.render(
                        Results=Results
                    )
                else:
                    return "<h2>没有找到数据！<h2>"
            except MySQLdb.Error, e:
                cursor.close()
                return "<h2>错误代码 %d: %s</h2>" % (e.args[0], e.args[1])
            except:
                cursor.close()
                return "<h2>未知错误！<h2>"

    #*******************登陆管理*******************

    def Login(self):
        UID = self.form.getfirst("UID")
        UID = cgi.escape(UID, True)
        PassWd = self.form.getfirst("password")
        PassWd = cgi.escape(PassWd, True)
        sql = 'SELECT UserName, Auth FROM Users WHERE UID = "%s" AND PassWd = "%s"' \
              % (UID, PassWd)
        conn = self.ConnectMySQL()
        cursor = conn.cursor()
        cursor.execute(sql)
        results = cursor.fetchall()
        if results:
            # 更新对象的当前状态
            self.UserName = results[0][0]
            self.Auth = results[0][1]
            self.UID = UID
            # 生成一个有效的Session ID，发送给客户端
            SID = sha.new(repr(time.time()) + str(random())).hexdigest()
            cookie = SimpleCookie()
            cookie["SID"] = SID
            cookie["SID"]["path"] = '/'
            cookie["SID"]["expires"] = int(ConfigDict["cookieexpireseconds"])
            self.HTML_Headers = "%s\r\n%s" % (cookie, self.HTML_Headers)
            # 同时将新生成的SID插入数据库
            sql = 'UPDATE Users SET SID = "%s", LastLogin = Now() WHERE UID = "%s"' % (SID, self.UID)
            cursor.execute(sql)
            conn.commit()
            cursor.close()
        # 最后返回一个自动刷新的页面
        # 即使登陆不成功也可以这样返回
            return self.RefreshPage(u"<h1>登陆成功</h1>")
        else:
            return self.RefreshPage(u"<h1>登陆失败</h1>")

    def LogOut(self):
        sql = 'UPDATE Users SET SID = NULL WHERE UID = "%s"' % self.UID
        conn = self.ConnectMySQL()
        cursor = conn.cursor()
        cursor.execute(sql)
        conn.commit()
        cursor.close()
        return self.RefreshPage(u"<h1>注销成功</h1>")

    def CheckLoggedIn(self):
        # 首先读取并检查Cookie
        cookie_string = environ.get("HTTP_COOKIE")
        if cookie_string:
            cookie = SimpleCookie()
            cookie.load(cookie_string)
            SID = cookie["SID"].value

        else:
            return
        # 然后连接数据库
        sql = 'SELECT UserName, Auth, UID, LastLogin FROM Users WHERE SID="%s"' % SID
        conn = self.ConnectMySQL()
        cursor = conn.cursor()
        cursor.execute(sql)
        results = cursor.fetchall()
        if results:
            self.UserName = results[0][0]
            self.Auth = results[0][1]
            self.UID = results[0][2]
            sys.stderr.write(str(results[0][3]))
            self.TimeLeft = int(ConfigDict["cookieexpireseconds"]) - \
                            int((datetime.now() - results[0][3]).total_seconds())

    #*******************常规业务*******************

    def BorrowBook(self):
        pass

    def ReturnBook(self):
        pass

    def RenewalBook(self):
        pass

    #*******************管理功能*×******************

    def ManageBibliography(self):
        if self.Auth == -1:
            return "请先登陆！"
        elif self.Auth == 1:
            return "普通用户无权操作书目！"
        elif self.Auth == 2:
            return "被封禁用户无权操作书目！"
        elif self.Auth == 0:
            ISBN = self.form.getfirst("ISBN")
            BookName = self.form.getfirst("BookName")
            Author = self.form.getfirst("Author")
            Publisher = self.form.getfirst("Publisher")
            PubDate = self.form.getfirst("PubDate")
            try:
                TimeLimit = int(self.form.getfirst("TimeLimit"))
            except ValueError:
                return "数据无效！"
            subaction = self.form.getfirst("subaction")

            if subaction == "Add":
                sql = 'INSERT INTO Bibliography VALUES ("%s", "%s", "%s", "%s", "%s", %d, Now())' \
                  % (ISBN, BookName, Author, Publisher, PubDate, TimeLimit)
            elif subaction == "Del":
                sql = 'DELETE FROM Bibliography WHERE ISBN = "%s"' % ISBN
            elif subaction == "Modify":
                sql = 'UPDATE Bibliography SET BookName = "%s", Author = "%s", Publisher = "%s", ' \
                  'PubDate = "%s", TimeLimit = %d, LastVis = Now() WHERE ISBN = "%s"' \
                  % (BookName, Author, Publisher, PubDate, TimeLimit, ISBN)
            else:
                return "未知操作！"

            conn = self.ConnectMySQL()
            cursor = conn.cursor()
            try:
                cursor.execute(sql)
                conn.commit()
            except MySQLdb.Error, e:
                conn.rollback()
                cursor.close()
                return "错误代码 %d: %s" % (e.args[0], e.args[1])
            except:
                conn.rollback()
                cursor.close()
                return "未知错误！"
            else:
                cursor.close()
                return "操作成功！"

    def ManageBook(self):
        pass

    def ManageUser(self):
        pass

    #*******************辅助函数*×******************

    def ConnectMySQL(self):
        try:
            conn = MySQLdb.connect(
                host=ConfigDict["db_server"],
                user=ConfigDict["db_user"],
                passwd=ConfigDict["db_passwd"],
                db=ConfigDict["db_name"]
            )
            conn.set_character_set('UTF8')
            return conn
        except:
            print self.HTML_Headers
            print self.RefreshPage('<h1 style="color:red">Error when connecting to MySQL, Exit!</h1>')
            sys.exit(-1)

    def ReadCode(self):
        try:
            device = ConfigDict["device"]
            print device
            # create a Processor
            proc = Processor()
            # configure the Processor
            proc.parse_config('enable')
            # initialize the Processor
            proc.init(device)
            proc.visible = True
            proc.process_one()
            proc.visible = False
        except:
            return "Error! Could not capture figure!"
        # extract results
        result = ""
        for symbol in proc.results:
            # do something useful with results
            result += symbol.data + '\n'
            # print 'decoded', symbol.type, 'symbol', '"%s"' % symbol.data
        if not result: result = "Error! Could not recognize code!"
        return result

    def QueryISBN(self):
        StrISBN = self.form.getfirst('ISBN', "")
        try:
            fid = urlopen(ConfigDict["isbn_query_url"] +StrISBN)
            json_data = fid.read()
            return json_data
        except:
            return json.dumps({
                "title": "Unknown",
                "author": ["Unknown"],
                "publisher": "Unknown",
                "pubdate": "Unknown",
            })

if __name__ == "__main__":
    # environ["QUERY_STRING"] = "action=Login&UID=010022011021&password=1994.2.21"
    LibraryManager()

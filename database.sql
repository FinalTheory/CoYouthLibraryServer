Create Database TinyLibrary Character Set 'utf8';

use TinyLibrary;

/*
	允许自定义昵称，但是使用学号作为主键；
	SessionID存储于用户表中；
	并且保存每个用户的权限信息：0表示管理员，1表示普通用户，2表示被封禁用户；
	以及存储用户姓名，学院，联系方式等信息，这里要求联系方式不同。
	另外，定义一个结构完全相同的用户列表，用来存储所有申请注册的用户。
*/

Create Table Users
(
	UID				varchar(15) Primary Key,
	PassWd		varchar(64)	Not NULL,
	SID				varchar(64) Unique Default NULL,
	Auth			TinyInt			Default 1,
	Email			varchar(32) Unique,
	Tel				varchar(20)	Unique,
	LastLogin	DateTime 		Default NULL,
	UserName	varchar(15)	Default 'ACMer'
) Default Charset=utf8;

Create Table RegisterUsers
(
	UID				varchar(15) Primary Key,
	PassWd		varchar(64)	Not NULL,
	SID				varchar(64) Unique Default NULL,
	Auth			TinyInt			Default 1,
	Email			varchar(32) Unique,
	Tel				varchar(20)	Unique,
	LastLogin	DateTime 		Default NULL,
	UserName	varchar(15)	Default 'ACMer'
) Default Charset=utf8;

/*
	描述图书分类的表，以图书的ISBN作为唯一标识；
	仅仅简单记录了图书的书名，作者和出版商；
	记录每一种图书可以借阅多少天，0为默认，表示不限。
	最后还要记录一下这种图书上次是什么时候被访问的，以便于增添新书。
*/

Create Table Bibliography
(
	ISBN			varchar(20) 	Primary Key,
	BookName	varchar(128)	Default NULL,
	Author		varchar(128)	Default NULL,
	Publisher varchar(128)	Default NULL,
	PubDate		varchar(20)		Default	NULL,
	TimeLimit	SmallInt			Default	0,
	LastVis		DateTime			Default NULL
) Default Charset=utf8;


/*
	使用BookID描述一本书唯一的编号；
	使用ISBN描述一本书的具体属性；
	每本书被用户表中UserID对应的用户借阅；
	定义书籍当前状态，0表示借出，1表示可用，-1表示暂时丢失。
	记录图书过期时间，便于进行邮件提醒。
*/

Create Table Books
(
	BookID	INT 					Primary Key,
	Status	TinyInt 			Check( Status >= -1 and Status <= 1 ),
	Expire	DateTime			Default NULL,
	ISBN		varchar(20),
	UserID	varchar(15),
	Foreign Key (ISBN) 		References Bibliography(ISBN),
	Foreign Key (UserID)	References Users(UID)
) Default Charset=utf8;


/* 为常用的查询列创建索引 */
CREATE UNIQUE INDEX UserSID_Index ON Users (SID);


Insert Into Users (UserName, Passwd, UID, Auth, Email, Tel) Values("未借出", "", "0", 1, "", "");
Insert Into Users (UserName, Passwd, UID, Auth, Email, Tel) Values("FinalTheory", "1994.2.21", "010022011021", 0, "852301601@qq.com", "15689100026");
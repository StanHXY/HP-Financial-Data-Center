# Development framework header file path.
PUBINCL = -I/project/public

# Development framework cpp file, directly included here, not linked as a library for ease of debugging.
PUBCPP = /project/public/_public.cpp

# MySQL header file directory path
MYSQLINCL = -I/usr/local/mysql/include -I/project/public/db/mysql

# MySQL library file directory path
MYSQLLIB = -L/usr/local/mysql/lib

# MySQL link library
MYSQLLIBS = -lmysqlclient

# Development framework MySQL cpp file, directly included here, not linked as a library for ease of debugging.
MYSQLCPP = /project/public/db/mysql/_mysql.cpp

# Compilation flags.
CFLAGS = -g


all: procctl checkproc gzipfiles deletefiles ftpgetfiles ftpputfiles tcpputfiles fileserver\
     tcpgetfiles execsql dminingmysql xmltodb syncupdate syncincrement syncincrementex\
     deletetable migratetable xmltodb_oracle deletetable_oracle migratetable_oracle\
     dminingoracle syncupdate_oracle syncincrement_oracle syncincrementex_oracle\
     webserver inetd rinetd rinetdin 

procctl:procctl.cpp
	g++ -o procctl procctl.cpp
	cp procctl ../bin/.

checkproc:checkproc.cpp
	g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp checkproc ../bin/.

gzipfiles:gzipfiles.cpp
	g++ $(CFLAGS) -o gzipfiles gzipfiles.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp gzipfiles ../bin/.

deletefiles:deletefiles.cpp
	g++ $(CFLAGS) -o deletefiles deletefiles.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp deletefiles ../bin/.

ftpgetfiles:ftpgetfiles.cpp
	g++ $(CFLAGS) -o ftpgetfiles ftpgetfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc
	cp ftpgetfiles ../bin/.

ftpputfiles:ftpputfiles.cpp
	g++ $(CFLAGS) -o ftpputfiles ftpputfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc
	cp ftpputfiles ../bin/.

tcpputfiles:tcpputfiles.cpp
	g++ $(CFLAGS) -o tcpputfiles tcpputfiles.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp tcpputfiles ../bin/.

fileserver:fileserver.cpp
	g++ $(CFLAGS) -o fileserver fileserver.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp fileserver ../bin/.

tcpgetfiles:tcpgetfiles.cpp
	g++ $(CFLAGS) -o tcpgetfiles tcpgetfiles.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp tcpgetfiles ../bin/.

execsql:execsql.cpp
	g++ $(CFLAGS) -o execsql execsql.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp execsql ../bin/.

dminingmysql:dminingmysql.cpp
	g++ $(CFLAGS) -o dminingmysql dminingmysql.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp dminingmysql ../bin/.

xmltodb:xmltodb.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o xmltodb xmltodb.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp xmltodb ../bin/.

xmltodb_oracle:xmltodb_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o xmltodb_oracle xmltodb_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp xmltodb_oracle ../bin/.

deletetable_oracle:deletetable_oracle.cpp 
	g++ $(CFLAGS) -o deletetable_oracle deletetable_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp deletetable_oracle ../bin/.

dminingoracle:dminingoracle.cpp 
	g++ $(CFLAGS) -o dminingoracle dminingoracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp dminingoracle ../bin/.

migratetable_oracle:migratetable_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o migratetable_oracle migratetable_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp migratetable_oracle ../bin/.

syncupdate_oracle:syncupdate_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncupdate_oracle syncupdate_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp syncupdate_oracle ../bin/.

syncincrement_oracle:syncincrement_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncincrement_oracle syncincrement_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp syncincrement_oracle ../bin/.

syncincrementex_oracle:syncincrementex_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncincrementex_oracle syncincrementex_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lm -lc
	cp syncincrementex_oracle ../bin/.

syncupdate:syncupdate.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncupdate syncupdate.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp syncupdate ../bin/.

syncincrement:syncincrement.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncincrement syncincrement.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp syncincrement ../bin/.

syncincrementex:syncincrementex.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncincrementex syncincrementex.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp syncincrementex ../bin/.

deletetable:deletetable.cpp
	g++ $(CFLAGS) -o deletetable deletetable.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp deletetable ../bin/.

migratetable:migratetable.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o migratetable migratetable.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp migratetable ../bin/.

webserver:webserver.cpp 
	g++ $(CFLAGS) -o webserver webserver.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lpthread -lm -lc
	cp webserver ../bin/.

inetd:inetd.cpp
	g++ $(CFLAGS) -o inetd inetd.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp inetd ../bin/.

rinetd:rinetd.cpp
	g++ $(CFLAGS) -o rinetd rinetd.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp rinetd ../bin/.

rinetdin:rinetdin.cpp
	g++ $(CFLAGS) -o rinetdin rinetdin.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp rinetdin ../bin/.

clean:
	rm -f procctl checkproc gzipfiles deletefiles ftpgetfiles ftpputfiles tcpputfiles fileserver
	rm -f tcpgetfiles execsql dminingmysql xmltodb syncupdate syncincrement syncincrementex
	rm -f deletetable migratetable xmltodb_oracle deletetable_oracle migratetable_oracle
	rm -f dminingoracle syncupdate_oracle syncincrement_oracle syncincrementex_oracle
	rm -f webserver inetd rinetd rinetdin 

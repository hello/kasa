#!/usr/bin/python
# argv[1] :username, argv[2] : password  argv[3]: shadow file dir
import crypt, sys, os, random, re, string

username = sys.argv[1]
password = sys.argv[2]
shadow_dir = sys.argv[3]

salt = string.join(random.sample(['a','b','c','d','e','f','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0'],16)).replace(" ","")
salt_string = '$6$' + salt + '$'

digest_pass = crypt.crypt(password, salt_string)

shadow_line= username + ':' + digest_pass + ':15874:0:99999:7:::'
passwd_line = username + ':x:1001:1001:Linux User,,,:/home/' + username  + ':/bin/sh'
home_dir = shadow_dir + '/home/' + username
#print 'shadow_line is: '  + shadow_line
#print 'passwd_line is: '	+ passwd_line
#append the user to /etc/passwd

file_name= shadow_dir + '/etc/passwd'
old_file = open(file_name, 'r')
new_file = open(file_name + '~', 'w')
found = 0
for line in old_file:
	if line.startswith(username) :
		found = 1
	line = re.sub( username  + ':.+', passwd_line,line)
	new_file.write(line)

if (found == 0) :
	new_file.write(passwd_line)
new_file.close()
old_file.close()
os.rename(file_name + '~', file_name)

#append the user to /etc/shadow with password
file_name= shadow_dir + '/etc/shadow'
old_file = open(file_name, 'r')
new_file = open(file_name + '~', 'w')
found = 0
for line in old_file:
	if line.startswith(username) :
		found = 1
	line = re.sub( username  + ':.+', shadow_line,line)
	new_file.write(line)

if (found == 0) :
	new_file.write(shadow_line)
new_file.close()
old_file.close()
os.rename(file_name + '~', file_name)

#create user home dir if not exist
if not os.path.exists(home_dir):
	os.makedirs(home_dir)

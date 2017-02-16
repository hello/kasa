#!/usr/bin/python
import crypt, sys, os, random, re, string

salt = string.join(random.sample(['a','b','c','d','e','f','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0'],16)).replace(" ","")
salt_string = '$6$' + salt + '$'
password = sys.argv[1]

digest_pass = crypt.crypt(password, salt_string)

shadow_line='root:' + digest_pass + ':15874:0:99999:7:::'

file_name=sys.argv[2] + '/etc/shadow'
print 'file_name is' + file_name

old_file = open(file_name, 'r')
new_file = open(file_name + '~', 'w')
for line in old_file:
	line = re.sub(r'root:.+', shadow_line,line)
	new_file.write(line)
new_file.close()
old_file.close()
os.rename(file_name + '~', file_name)





#!/usr/bin/python
import crypt, sys, os, random, re
value=random.randint(11,99)
encrypt_pass=crypt.crypt(sys.argv[1],str(value))
shadow_line='root:' + encrypt_pass + ':15874:0:99999:7:::'

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





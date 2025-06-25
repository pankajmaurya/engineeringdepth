fname1='f1'
fname2='f2'

import random
random_line = random.randint(5000, 10000)

print(f'Writing 2 files with 10k lines differing on line {random_line}')
with open(fname1, 'a') as f1:
	for i in range(1, 10001):
		f1.write(f'This is line {i}\n')

with open(fname2, 'a') as f2:
	for i in range(1, 10001):
		if i == random_line:
			f2.write(f'Changed line {i}\n')
		else:
			f2.write(f'This is line {i}\n')



fname1='f1'
fname2='f2'
num_lines = 1000000
mid = int(num_lines / 2)
import random
random_line = random.randint(mid, num_lines)

print(f'Writing 2 files with 10k lines differing on line {random_line}')
with open(fname1, 'w') as f1:
	for i in range(1, num_lines + 1):
		f1.write(f'This is line {i}\n')

with open(fname2, 'w') as f2:
	for i in range(1, num_lines + 1):
		if i == random_line:
			f2.write(f'Changed line {i}\n')
		else:
			f2.write(f'This is line {i}\n')



import os
from statistics import stdev

def main():
	for file in os.listdir():
		if file.endswith(".txt") and file.startswith('E'):
			print(file)
			avg_rss = 0
			rss = []
			num = 0
			with open(file, 'r', encoding='utf-8') as f:
				text = f.read()
				lines = text.split('\n')
				fields = [i.split('\t') for i in lines]
				fields.pop()
				num = len(fields)
				for i in fields:
					val = int(i[2])
					avg_rss += val
					rss.append(val)
			avg_rss /= num
			with open('results.txt', 'a', encoding='utf-8') as f:
				f.write(file)
				f.write('\n')
				f.write(str(num/10))
				f.write('\t')
				f.write(str(round(avg_rss,1) ))
				f.write('\t')
				f.write(str(round(stdev(rss), 2 )))
				f.write('\n')

if __name__ == '__main__':
	main()

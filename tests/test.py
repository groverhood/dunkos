
import sys
import argparse

def main():
	args = sys.argv[1:]

	parser = argparse.ArgumentParser()
	
	parser.parse_args(args)
	

if __name__ == '__main__':
	main()

import argparse
import subprocess


from glob import glob
from os import listdir, scandir
from os.path import dirname, isdir, abspath, splitext


class TestCase(object):
	""" Object which performs tests over directories that store at least
		a single C test source, and has functionality to write the unit
		results, and then compare them to the kernel's results. """

	compiler_path: str

	def __init__(self, testdir: str):

		# We want a directory which has AT LEAST a single
		# C test source.
		assert isdir(testdir)
		testdir_cglob = glob(f'{testdir}/*.c')
		assert len(testdir_cglob) is not 0
		
		# We only want a single C test source
		[testsrc_rel, *_] = testdir_cglob
		self.__testsrc = abspath(testsrc_rel)

		self.generate_expected_output()
		self.generate_test_output()

	def generate_expected_output(self):
		""" Generate the expected output using the real machine. """

		gcc = subprocess.Popen(('gcc', '-o /dev/stdout', '-O2', f'{self.__testsrc}'), stdout=subprocess.PIPE)
		output = subprocess.check_output(('bash'), stdin=gcc.stdout)
		gcc.wait()

		name, _ = splitext(self.__testsrc)

		with open(name + '.output', 'w') as output_file:
			output_file.write(output)

	def generate_test_output(self):
		pass

	def __repr__(self):
		return ''


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('-r', '--root', type=str, dest='rootdir', default=dirname(__file__))
	parser.add_argument('-cc', '--compiler', type=str, dest='compiler')

	args = parser.parse_args()

	TestCase.compiler_path = args.compiler
	tests = map(filter(listdir(args.rootdir), isdir), TestCase)

	print(*tests, sep='\n')
	

if __name__ == '__main__':
	main()
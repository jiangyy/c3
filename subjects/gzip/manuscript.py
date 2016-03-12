import os

# This is everything you need to validate GNU zip!
# You will observe inconsistent crash site for gzip.

# a function decorated by @prepare will be invoked
# before crash consistency validation.
@prepare
def handler():
  fp = open("file.txt", "w")
  fp.write(str(2**32768)) # anything can trigger the bug
  fp.close()

# a function decorated by @run_program indicates
# the program run. You can also directly put
# Python scripts here.
@run_program
def handler():
  os.system("gzip file.txt")

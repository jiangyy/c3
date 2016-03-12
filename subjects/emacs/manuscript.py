import os

# This is everything you need to validate Emacs!
# Emacs is crash-safe.

# a function decorated by @prepare will be invoked
# before crash consistency validation.
@prepare
def handler():
  fp = open("file.txt", "w")
  for i in range(0, 100):
    fp.write('Line %d of the file.\n' % i)
  fp.close()


# a function decorated by @run_program indicates
# the program run. You can also directly put
# Python scripts here.
@run_program
def handler():
  system("emacs -nw file.txt")

# A function decorated by @after($t) will be invoked
# after $t seconds of the program start.
# This is an example for edting a file using emacs
@after(3)
def do_sth():
  keypress('ctrl-x'); keypress('h'); # select all text
  sleep(1)

  keypress('alt-w'); # copy
  sleep(1)

  # paste for three times
  for i in range(0, 3):
    keypress('ctrl-y'); sleep(1)

  keypress('ctrl-x'); keypress('ctrl-c'); keypress('y') # save and exit

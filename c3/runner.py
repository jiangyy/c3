"""
Three decorators are supproted: 
  prepare: prepare scripts
  run_program: execute function as the program being tested (e.g., via os.system)
  after(delay): execute function after delay seconds

Usage:
  @prepare
  def do_sth(): ...

  @run_program
  def do_sth(): ...

  @after(delay):
  def do_sth(): ...

Useful APIs:
  keypress(s)
"""

import sys, ctypes, os, time, pyautogui
from os import system
from time import sleep
from multiprocessing import Process

queue = []
proc = None
start_tsc = None

def keypress(cmd):
  FNKEY = ['ctrl', 'alt']
  keys = cmd.lower().split('-')
  for key in keys:
    if key in FNKEY:
      pyautogui.keyDown(key)
    else:
      pyautogui.press(key)
  for key in keys:
    if key in FNKEY:
      pyautogui.keyUp(key)

def after(delay):
  def _wrap(func):
    global queue
    last = max([i[0] for i in queue] + [1.0])
    queue.append((last + delay, func))
  return _wrap

def prepare(func):
  global queue
  queue.append((-1.0, func))

def run_program(func):
  global queue
  queue.append((0.0, func))

def fire_events(delays = [], subset = lambda x: True):
  global proc, start_tsc

  q = filter(subset, queue)
  q.sort(key = lambda x: x[0])
  for (tsc, func) in q:
    if tsc < 0:
      func()
    elif tsc == 0:
      proc = Process(target = func)
      proc.start()
      start_tsc = time.time()
    else:
      ctsc = time.time()
      wt = tsc - (ctsc - start_tsc)
      if wt > 0:
        time.sleep(wt)
      if not proc.is_alive(): 
        return
      func()

if __name__ == '__main__':
  try:
    (path, mode) = sys.argv[1:]
  except:
    print "[Runner] Usage: runner path mode"
    exit(1)

  os.chdir(path)

  execfile('manuscript.py')

  if mode == 'prepare':
    fire_events(subset = lambda t: t[0] < 0)
    os.system("sync; sync")
  elif mode == 'run':
    fire_events(subset = lambda t: t[0] >= 0)
    if proc is not None:
      proc.join()

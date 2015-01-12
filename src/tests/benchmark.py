#!/usr/bin/env python3

import config, sh, shutil, os

# TODO: let user specify the tests to run on CLI
tests = config.TESTS

def findImplementations(tests):
  implementation_names = set()
  for case in tests.values():
    for instances in case["tests"].values():
      implementation_names.add(instances["implementation"])
  implementations = {k : config.IMPLEMENTATIONS[k] for k in implementation_names}
  return implementations

def findBuildTargets(implementations):
  build_targets = {}
  for implementation in implementations.values():
    version = implementation["version"]
    existing_targets = build_targets.get(version, set())
    existing_targets.add(implementation["target"])
    build_targets[version] = existing_targets
  return build_targets

def clean():
  try:
    shutil.rmtree(config.WORKING_DIRECTORY)
  except FileNotFoundError:
    # ignore: if directory doesn't exist, no need to delete it
    pass
  os.mkdir(config.WORKING_DIRECTORY)

def gitCheckout(version):
  directory = os.path.join(config.WORKING_DIRECTORY, version)
  os.mkdir(directory)
  # how to checkout branch to new working directory, see:
  # htp://blog.jessitron.com/2013/10/git-checkout-multiple-branches-at-same.html
  sh.tar(sh.git.archive(version), "-xC", directory)
  
def build(version, targets):
  saved_cwd = os.getcwd()
  
  directory = os.path.join(config.WORKING_DIRECTORY, version)
  build_directory = os.path.join(directory, config.BUILD_ROOT)
  source_directory = os.path.join(directory, config.SOURCE_ROOT) 
  
  os.mkdir(build_directory)
  os.chdir(build_directory)
  # TODO: logging?
  sh.cmake(source_directory)
  sh.make(*targets)
  
  os.chdir(saved_cwd)
  
def buildImplementations(implementations):
  build_targets = findBuildTargets(implementations)
  
  clean()
  for version in build_targets:
    gitCheckout(version)
    build(version, build_targets[version])
    
os.chdir(config.PROJECT_ROOT)

implementations = findImplementations(tests)
buildImplementations(implementations)
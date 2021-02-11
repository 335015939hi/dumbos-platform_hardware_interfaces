#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Dump new HALs that are introduced in each FCM version in a human-readable format.

Example:
hals_for_release.py
  Show changes for each release, including new and deprecated HALs.
hals_for_release.py -dua
  Show changes as well as unchanged HALs for each release.
hals_for_release.py -i
  Show details about instance names and regex patterns as well.
hals_for_release.py -p wifi
  Show changes of Wi-Fi HALs for each release.
"""

import argparse
import enum
import logging
import os
import subprocess
import sys

logging.basicConfig(format="%(levelname)s: %(message)s")
logger = logging.getLogger(__name__)


def ParseArgs():
  """
  Parse arguments.
  :return: arguments.
  """
  parser = argparse.ArgumentParser(description=__doc__,
                                   formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument("--analyze-matrix", help="Location of analyze_matrix")
  parser.add_argument("input", metavar="INPUT", nargs="?",
                      help="Directory of compatibility matrices.")
  parser.add_argument("--deprecated", "-d",
                      help="Show deprecated HALs. If none of deprecated, unchanged or introduced "
                           "is specified, default is --deprecated and --introduced",
                      action="store_true")
  parser.add_argument("--unchanged", "-u",
                      help="Show unchanged HALs. If none of deprecated, unchanged or introduced "
                           "is specified, default is --deprecated and --introduced",
                      action="store_true")
  parser.add_argument("--introduced", "-a",
                      help="Show deprecated HALs. If none of deprecated, unchanged or introduced "
                           "is specified, default is --deprecated and --introduced",
                      action="store_true")
  parser.add_argument("--instances", "-i", action="store_true",
                      help="Show instance names and regex patterns as well")
  parser.add_argument("--packages", "-p", nargs="*", metavar="PACKAGE",
                      help="Only print HALs where package contains the given substring. "
                           "E.g. wifi, usb, health. Recommend to use with --unchanged.")
  parser.add_argument("--verbose", "-v", action="store_true", help="Verbose mode")
  args = parser.parse_args()

  if args.verbose:
    logger.setLevel(logging.DEBUG)

  if not args.deprecated and not args.unchanged and not args.introduced:
    args.deprecated = args.introduced = True

  host_out = os.environ.get("ANDROID_HOST_OUT")
  if host_out and not args.analyze_matrix:
    analyze_matrix = os.path.join(host_out, "bin", "analyze_matrix")
    if os.path.isfile(analyze_matrix):
      args.analyze_matrix = analyze_matrix
  if not args.analyze_matrix:
    args.analyze_matrix = "analyze_matrix"

  top = os.environ.get("ANDROID_BUILD_TOP")
  if top and not args.input:
    args.input = os.path.join(top, "hardware", "interfaces", "compatibility_matrices")
  if not args.input:
    args.input = os.path.dirname(os.path.realpath(__file__))

  logger.debug("Using analyze_matrix at path: %s", args.analyze_matrix)
  logger.debug("Dumping compatibility matrices at path: %s", args.input)
  logger.debug("Show deprecated HALs? %s", args.deprecated)
  logger.debug("Show unchanged HALs? %s", args.unchanged)
  logger.debug("Show introduced HALs? %s", args.introduced)
  logger.debug("Only showing packages %s", args.packages)

  return args


def Analyze(analyze_matrix, file, args, ignore_errors=False):
  """
  Run analyze_matrix with
  :param analyze_matrix: path of analyze_matrix
  :param file: input file
  :param arg: argument to analyze_matrix, e.g. "level"
  :param ignore_errors: Whether errors during execution should be rased
  :return: output of analyze_matrix
  """
  command = [analyze_matrix, "--input", file] + args
  proc = subprocess.run(command,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.DEVNULL if ignore_errors else subprocess.PIPE)
  if not ignore_errors and proc.returncode != 0:
    logger.warning("`%s` exits with code %d with the following error: %s", " ".join(command),
                   proc.returncode, proc.stderr)
    proc.check_returncode()
  return proc.stdout.decode().strip()


def GetLevel(analyze_matrix, file):
  """
  :param analyze_matrix: Path of analyze_matrix
  :param file: a file, possibly a compatibility matrix
  :return: If it is a compatibility matrix, return an integer indicating the level.
    If it is not a compatibility matrix, returns None.
    For matrices with empty level, return None.
  """
  output = Analyze(analyze_matrix, file, ["--level"], ignore_errors=True)
  # Ignore empty level matrices and non-matrices
  if not output:
    return None
  try:
    return int(output)
  except ValueError:
    logger.warning("Unknown level '%s' in file: %s", output, file)
    return None


def GetLevelName(analyze_matrix, file):
  """
  :param analyze_matrix: Path of analyze_matrix
  :param file: a file, possibly a compatibility matrix
  :return: If it is a compatibility matrix, return the level name.
    If it is not a compatibility matrix, returns None.
    For matrices with empty level, return "Level unspecified".
  """
  return Analyze(analyze_matrix, file, ["--level-name"], ignore_errors=True)


def ReadMatrices(args):
  """
  :param args: parsed arguments from ParseArgs
  :return: A dictionary. Key is an integer indicating the matrix level.
  Value is (level name, a set of instances in that matrix).
  """
  matrices = dict()
  for child in os.listdir(args.input):
    file = os.path.join(args.input, child)
    level, level_name = GetLevel(args.analyze_matrix, file), GetLevelName(args.analyze_matrix, file)
    if level is None:
      logger.debug("Ignoring file %s", file)
      continue
    action = "--instances" if args.instances else "--interfaces"
    instances = Analyze(args.analyze_matrix, file, [action, "--requirement"]).split("\n")
    instances = map(lambda l: l.strip(), instances)
    instances = filter(lambda l: l, instances)
    instances = set(instances)
    if level in matrices:
      logger.warning("Found duplicated matrix for level %s, ignoring: %s", level, file)
      continue
    matrices[level] = (level_name, instances)

  return matrices


class HalFormat(enum.Enum):
  HIDL = 0
  AIDL = 2


def GetHalFormat(instance):
  """
  Guess the HAL format of instance.
  :param instance: two formats:
    android.hardware.health.storage@1.0::IStorage/default optional
    android.hardware.health.storage.IStorage/default (@1) optional
  :return: HalFormat.HIDL for the first one, HalFormat.AIDL for the second.

  >>> str(GetHalFormat("android.hardware.health.storage@1.0::IStorage/default optional"))
  'HalFormat.HIDL'
  >>> str(GetHalFormat("android.hardware.health.storage.IStorage/default (@1) optional"))
  'HalFormat.AIDL'
  """
  return HalFormat.HIDL if "::" in instance else HalFormat.AIDL


def SplitInstance(instance):
  """
  Split instance into parts.
  :param instance:
  :param instance: two formats:
    android.hardware.health.storage@1.0::IStorage/default optional
    android.hardware.health.storage.IStorage/default (@1) optional
  :return: (package, version+interface+instance, requirement)

  >>> SplitInstance("android.hardware.health.storage@1.0::IStorage/default optional")
  ('android.hardware.health.storage', '@1.0::IStorage/default', 'optional')
  >>> SplitInstance("android.hardware.health.storage.IStorage/default (@1) optional")
  ('android.hardware.health.storage', 'IStorage/default (@1)', 'optional')
  """
  format = GetHalFormat(instance)
  if format == HalFormat.HIDL:
    atPos = instance.find("@")
    spacePos = instance.rfind(" ")
    return instance[:atPos], instance[atPos:spacePos], instance[spacePos + 1:]
  elif format == HalFormat.AIDL:
    dotPos = instance.rfind(".")
    spacePos = instance.rfind(" ")
    return instance[:dotPos], instance[dotPos + 1:spacePos], instance[spacePos + 1:]


def GetPackage(instance):
  """
  Guess the package of instance.
  :param instance: two formats:
    android.hardware.health.storage@1.0::IStorage/default
    android.hardware.health.storage.IStorage/default (@1)
  :return: The package. In the above example, return android.hardware.health.storage

  >>> GetPackage("android.hardware.health.storage@1.0::IStorage/default")
  'android.hardware.health.storage'
  >>> GetPackage("android.hardware.health.storage.IStorage/default (@1)")
  'android.hardware.health.storage'
  """
  return SplitInstance(instance)[0]


def SortByKey(d):
  """
  Sort dictionary by key.
  :param d:
  :return: a list of tuples corresponding to pairs in the dictionary, sorted by the first element.

  >>> SortByKey({1:3, 4:2, 2:1})
  [(1, 3), (2, 1), (4, 2)]
  """
  return sorted(d.items(), key=lambda tup: tup[0])


def KeyOnPackage(instances):
  """
  :param instances: A list of instances.
  :return: A dictionary, where key is the package (see GetPackage), and
    value is a list of instances in the provided list, where
    GetPackage(instance) is the corresponding key.
  """
  d = dict()
  for instance in instances:
    package = GetPackage(instance)
    if package not in d: d[package] = []
    d[package].append(instance)
  return d


def GetReport(tuple1, tuple2, args):
  """
  :param tuple1: (level, (level_name, Set of instances from the first matrix))
  :param tuple2: (level, (level_name, Set of instances from the second matrix))
  :return: A human-readable report of their difference.
  """
  level1, (level_name1, instances1) = tuple1
  level2, (level_name2, instances2) = tuple2

  instances_by_package1 = KeyOnPackage(instances1)
  instances_by_package2 = KeyOnPackage(instances2)
  all_packages = set(instances_by_package1.keys()) | set(instances_by_package2.keys())

  if args.packages:
    package_matches = lambda package: any(pattern in package for pattern in args.packages)
    all_packages = filter(package_matches, all_packages)

  packages_report = dict()
  for package in all_packages:
    package_instances1 = set(instances_by_package1.get(package, []))
    package_instances2 = set(instances_by_package2.get(package, []))

    package_report = []
    deprecated = sorted(package_instances1 - package_instances2)
    unchanged = sorted(package_instances1 & package_instances2)
    introduced = sorted(package_instances2 - package_instances1)

    desc = lambda fmt, instance: fmt.format(GetHalFormat(instance).name, *SplitInstance(instance))

    if args.deprecated:
      package_report += [desc("- {0} {2} can no longer be used", instance)
                         for instance in deprecated]
    if args.unchanged:
      package_report += [desc("  {0} {2}", instance) for instance in unchanged]
    if args.introduced:
      package_report += [desc("+ {0} {2} is {3}", instance) for instance in introduced]

    if package_report:
      packages_report[package] = package_report

  report = ["============",
            "Level %s (%s) (against Level %s (%s))" % (level2, level_name2, level1, level_name1),
            "============"]
  for package, lines in SortByKey(packages_report):
    report.append(package)
    report += [("    " + e) for e in lines]

  return "\n".join(report)


def main():
  args = ParseArgs()
  matrices = ReadMatrices(args)
  sorted_matrices = SortByKey(matrices)
  if not sorted_matrices:
    logger.warning("Nothing to show, because no matrices found in '%s'.", args.input)
  for i in range(len(sorted_matrices) - 1):
    print(GetReport(sorted_matrices[i], sorted_matrices[i + 1], args))


if __name__ == "__main__":
  sys.exit(main())

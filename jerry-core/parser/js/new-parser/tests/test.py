#!/usr/bin/python3

import argparse, os, subprocess, sys

def die(message):
    print("[ERROR]: %s" % message)
    sys.exit(1)

def warn(message):
    print("[WARN]: %s" % message)

class TestResult:
    C_ALL_COLOR  = "\033[1m"
    C_SKIP_COLOR = "\033[1;34m"
    C_EXEC_COLOR = ""
    C_PASS_COLOR = "\033[1;32m"
    C_FAIL_COLOR = "\033[1;31m"
    C_ERR_COLOR  = "\033[1;35m"

    """A class to hold test results"""
    def __init__(self, verbose = False):
        self.num_all  = 0
        self.num_exec = 0
        self.num_pass = 0
        self.num_fail = 0
        self.num_err  = 0
        self.num_skip = 0
        self.verbose  = verbose

    def addPass(self, testcase):
        if self.verbose:
            print("%s[PASS] %s\033[0m" % (self.C_PASS_COLOR, testcase))
        self.num_pass += 1
        self.addExec()

    def addFail(self, testcase):
        print("%s[FAIL] %s\033[0m" % (self.C_FAIL_COLOR, testcase))
        self.num_fail += 1
        self.addExec()

    def addError(self, testcase):
        print("%s[ERR ] %s\033[0m" % (self.C_ERR_COLOR, testcase))
        self.num_err += 1
        self.addExec()

    def addExec(self):
        self.num_exec += 1
        self.addTest()

    def addSkip(self, testcase):
        print("%s[SKIP] %s\033[0m" % (self.C_SKIP_COLOR, testcase))
        self.num_skip += 1
        self.addTest()

    def addTest(self):
        self.num_all += 1

    def report(self):
        print("============================================================")
        print("%sAll tests . . . . . . . . . . . : %5d\033[0m"             % (self.C_ALL_COLOR, self.num_all))
        if self.verbose or self.num_skip > 0:
            print("%s  +-- skipped tests . . . . . . :     + %5d\033[0m"       % (self.C_SKIP_COLOR, self.num_skip))
        print("%s  +-- executed tests  . . . . . :     + %5d\033[0m"       % (self.C_EXEC_COLOR, self.num_exec))
        print("%s      +-- passed tests  . . . . :           + %5d\033[0m" % (self.C_PASS_COLOR, self.num_pass))
        if self.verbose or self.num_fail > 0:
            print("%s      +-- failed tests  . . . . :           + %5d\033[0m" % (self.C_FAIL_COLOR, self.num_fail))
        if self.verbose or self.num_err > 0:
            print("%s      +-- errors  . . . . . . . :           + %5d\033[0m" % (self.C_ERR_COLOR, self.num_err))
        print("============================================================")

argsparser = argparse.ArgumentParser(description='Execute js-parser tests.')
argsparser.add_argument('--binary',
                        action='store',
                        dest='javaScriptParser',
                        default='bin/js-parser')
argsparser.add_argument('--keep-output',
                        action='store_false',
                        dest='removeTestOutput',
                        default=True)
argsparser.add_argument('--tests',
                        action='append',
                        dest='javaScriptParserTests',
                        default=[])
argsparser.add_argument('--testbase',
                        action='store',
                        dest='javaScriptParserTestBaseDir',
                        default='.')
argsparser.add_argument('--verbose',
                        action='store_true',
                        dest='verboseOutput',
                        default=False)

args = argsparser.parse_args()
javaScriptParser=os.path.abspath(args.javaScriptParser)

os.path.isfile(javaScriptParser) or die('JS parser `' + args.javaScriptParser + "' does not exist.")
os.access(javaScriptParser, os.X_OK) or die('JS parser `' + args.javaScriptParser + "' is not executable.")
os.path.isdir(args.javaScriptParserTestBaseDir) or die('Test base directory `' + args.javaScriptParserTestBaseDir + "' does not exist.")

os.chdir(args.javaScriptParserTestBaseDir)

testList=[]

if not args.javaScriptParserTests:
    testList.append('js')
else:
    for testset in args.javaScriptParserTests:
        testList.extend(testset.split(','))

testCaseList=[]

while testList:
    test=testList.pop()
    if os.path.isfile(test):
        if test.endswith('.js'):
            testCaseList.append(test)
    else:
        for root, subdirs, files in os.walk(test):
            for f in files:
                if f.endswith('.js'):
                    testCaseList.append(os.path.join(root, f))

results = TestResult(args.verboseOutput)

for testCase in testCaseList:
    if not os.path.isfile(testCase + '.expected'):
        results.addSkip(testCase)
        continue
    with open(testCase+'.out', 'w') as out:
     with open(testCase+'.err', 'w') as err:
        testResult=subprocess.call([javaScriptParser, testCase], stdout=out, stderr=err, timeout=1)
        if testResult != 0:
            results.addError(testCase)
        elif subprocess.call(['cmp', '--quiet', testCase+'.expected', testCase+'.out']) != 0:
            results.addFail(testCase)
        else:
            results.addPass(testCase)
        if args.removeTestOutput:
            os.remove(testCase+'.out')
            os.remove(testCase+'.err')

results.report()

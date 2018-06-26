import psutil, os, random, subprocess, sys, signal

current_path = ""

test_base = "tests/"

ports_seen = set ()

def main (f, is_mem):
    if f:
        if is_mem:
            test_location = test_base + "memory/"
        else:
            test_location = test_base + "functionality/"
        filename = "{}{}".format (test_location, f)
        print filename
        if is_mem:
            parse_file (filename, "--m")
        else:
            parse_file (filename, "")
        run_test (filename, is_mem)
        diff (filename)
        if is_mem:
            display_valgrind (test_location, f)
    else:
        print "RUNNING FUNCTIONALITY TESTS:"
        sys.stdout.write ("\n\n\n\n")
        test_location = test_base + "functionality/"
        test_files = os.listdir (test_location)
        for f in test_files:
            filename = "{}{}".format (test_location, f)
            if not os.path.isdir (filename):
                parse_file (filename, "")
                run_test (filename, False)
        total_tests_functionality = 0
        passed_tests_functionality = 0
        for f in test_files:
            total_tests_functionality += 1
            if diff ("{}{}".format (test_location, f)):
                passed_tests_functionality += 1
        sys.stdout.write ("\n\n\n\nRESULTS:\n")
        print "{} tests ran. {} passed. {} failed".format (total_tests_functionality, passed_tests_functionality, total_tests_functionality - passed_tests_functionality)


        print "\n\n\n\nRUNNING MEMORY TESTS:"
        sys.stdout.write ("\n\n\n\n")
        test_location = test_base + "memory/"
        test_files = os.listdir (test_location)
        for f in test_files:
            filename = "{}{}".format (test_location, f)
            if not os.path.isdir (filename):
                parse_file (filename, "--m")
                run_test (filename, True)
        total_tests_mem = 0
        passed_tests_mem = 0
        for f in test_files:
            total_tests_mem += 1
            passed = 0
            if diff ("{}{}".format (test_location, f)):
                passed = 1
            passed = passed * display_valgrind (test_location, f)
            passed_tests_mem += passed
        sys.stdout.write ("\n\n\n\nRESULTS:\n")
        print "{} tests ran. {} passed. {} failed".format (total_tests_mem, passed_tests_mem, total_tests_mem - passed_tests_mem)

        total_tests = total_tests_mem + total_tests_functionality
        passed_tests = passed_tests_mem + passed_tests_functionality

        sys.stdout.write ("\n\n\n\n\n\nOVERALL RESULTS:\n\n")
        print "{} tests ran. {} passed. {} failed".format (total_tests, passed_tests, total_tests - passed_tests)

def display_valgrind (test_location, f):
    passed = 1
    valgrind_file = open ("{}{}_refs/valgrind_log.txt".format (test_location, f))
    valgrind_buffer = []
    need_report = False
    for line in valgrind_file:
        need_report = need_report or scan_valgrind_line (line)
        valgrind_buffer.append (line)
    print "Displaying any valgrind errors for test {}{}_outputs/server".format (test_location, f)
    if need_report:
        passed = 0
        for line in valgrind_buffer:
            print line,
    valgrind_file.close ()
    return passed

def parse_file (filename, flags):
    subprocess.call (["python2.7 {}parse.py {} {}".format (current_path, filename, flags)], shell=True)

def run_test (filename, mem):
    print "Running test {}".format (filename)
    os.mkdir ("{0}_outputs".format (filename))
    port = random.randint (10000, 60000)
    while port in ports_seen:
        port = random.randint (10000, 60000)
    ports_seen.add (port)
    if mem:
        commandline = "./{1}run_tests {0}_inputs/ {0}_outputs/ {0}_refs/ {2} --m".format (filename, current_path, port)
    else:
        commandline = "./{1}run_tests {0}_inputs/ {0}_outputs/ {0}_refs/ {2}".format (filename, current_path, port)
    subprocess.call ([commandline], shell=True)

def diff (filename):
    ref_dir = "{}_refs".format (filename)
    output_dir = "{}_outputs".format (filename)
    ref_files = os.listdir (ref_dir)
    output_files = os.listdir (output_dir)
    ref_files.sort ()
    output_files.sort ()
    print "Displaying difference for {}".format (filename)
    passed = True
    for i in range (len (output_files)):
        commandline = "diff {}/{} {}/{}".format (output_dir, output_files[i], ref_dir, ref_files[i])
        print "Checking difference in file {}".format (output_files[i])
        retval = subprocess.call ([commandline], shell=True)
        if retval != 0:
            passed = False
    return passed


def determine_test_location (path):
    global test_base, current_path
    index = path.rfind ("/")
    if index != -1:
        test_base = path[:index] + "/" + test_base
        current_path = path[:index] + "/"

def scan_valgrind_line (line):
    words = line.split ()
    if len (words) < 3:
        return False
    if words[1] == "LEAK" and words[2] == "SUMMARY:":
        return True
    if len (words) < 4:
        return False
    if words[1] == "ERROR" and words[2] == "SUMMARY:" and words[3] != "0":
        return True
    return False

def keyboard_handler (signum, frame):
    current_process = psutil.Process ()
    children = current_process.children ()
    for child in children:
        os.kill (child.pid, signal.SIGUSR1)
    print "Received exit from user"
    exit (1)

if __name__ == '__main__':
    signal.signal (signal.SIGINT, keyboard_handler)
    determine_test_location (sys.argv[0])
    if len(sys.argv) == 3:
        if sys.argv[2] == "--m":
            is_mem = True
        else:
            is_mem = False
        main (sys.argv[1], is_mem)
    else:
        main (None, None)

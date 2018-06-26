COMPILER = gcc

FLAGS = -g -Wall -std=c99

TESTING_FLAGS = -g -Wall

CUNIT = -L/home/ff/cs61c/cunit/install/lib -I/home/ff/cs61c/cunit/install/include -lcunit

CLIENT_C = client.c client_utils.c student_client.c client_server_utils.c

CLIENT_H = client.h client_utils.h student_client.h client_server_utils.h

SERVER_C = server.c server_utils.c client_server_utils.c user_utils.c commands.c command_utils.c 

SERVER_H = server.h server_utils.h client_server_utils.h user_utils.h commands.h command_utils.h

build: client server

build-testing: build-run-tests build-run-user

run-testing: clean-testing build-testing clean build
	@python2.7 testing/run_tests.py

run-mem-test: clean-testing build-testing clean build
	@python2.7 testing/run_tests.py $(TEST_NAME) --m

run-correctness-test: clean-testing build-testing clean build
	@python2.7 testing/run_tests.py $(TEST_NAME) --f


clean-testing: clean-tests
	@rm -f testing/run_tests;
	@rm -f testing/run_user;

clean-tests:
	@for d in testing/tests/functionality/*/ ; do \
		rm -rf "$$d"; \
	done ;
	@for d in testing/tests/memory/*/ ; do \
		rm -rf "$$d"; \
	done ;


build-run-tests: testing/run_tests.c
	@$(COMPILER) $(TESTING_FLAGS) -o testing/run_tests testing/run_tests.c

build-run-user:  testing/run_user.c
	@$(COMPILER) $(TESTING_FLAGS) -o testing/run_user testing/run_user.c


client: $(CLIENT_C) $(CLIENT_H)
	@$(COMPILER) $(FLAGS) -o client $(CLIENT_C)

server: $(SERVER_C) $(SERVER_H)
	@$(COMPILER) $(FLAGS) -o server $(SERVER_C)

clean:
	@rm -f client server

unit-test: clean-unit build-unit
	@./testing/unit_tests

clean-unit:
	@rm -f testing/unit_tests

build-unit: client_server_utils.c client_server_utils.h testing/unit_tests.c
	@$(COMPILER) $(TESTING_FLAGS) -o testing/unit_tests testing/unit_tests.c client_server_utils.c $(CUNIT)


.PHONY: build clean client server clean-unit build-unit unit-test build-testing run-testing clean-testing clean-tests build-run-tests build-run-user run-mem-test run-correctness-test 

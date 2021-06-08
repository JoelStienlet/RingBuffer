
all:
	@echo 'nothing to build: directly include the .c file as-is in your project. make examples tests coverage'
	
examples:
	make -C Example_1
	make -C Example_2
	make -C Example_3
	
tests:
	make -C Tests

coverage:
	make -C Tests coverage
	geninfo ./outputs/ -b ./Tests -o ./outputs/cov.info
	genhtml ./outputs/cov.info -o output_coverage

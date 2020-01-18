asn3: asn3.o
		gcc asn3.o -o asn3.out -lpthread

asn3.o: asn3.c
		gcc -c asn3.c -lpthread

clean:
	rm -f assignment_3_output_file.txt *.out *.o core

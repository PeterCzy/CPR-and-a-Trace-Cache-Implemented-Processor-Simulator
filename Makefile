p41:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=128 --lsq=64 --iq=32 --iqnp=1 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e100000000 -c401.bzip2_dryer_test.53.0.33.gz pk
p42:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=256 --lsq=128 --iq=64 --iqnp=2 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e100000000 -c401.bzip2_dryer_test.53.0.33.gz pk
p43:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=512 --lsq=256 --iq=128 --iqnp=4 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e100000000 -c401.bzip2_dryer_test.53.0.33.gz pk
p51:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=128 --lsq=64 --iq=32 --iqnp=1 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e10000000 -c429.mcf_test.7.gz pk
p52:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=256 --lsq=128 --iq=64 --iqnp=2 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e10000000 -c429.mcf_test.7.gz pk
p53:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=512 --lsq=256 --iq=128 --iqnp=4 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m2048 -e10000000 -c429.mcf_test.7.gz pk
p91:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=128 --lsq=64 --iq=32 --iqnp=1 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m4096 -e100000000 -c600.perlbench_s_rand_test.21.gz pk
p92:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=256 --lsq=128 --iq=64 --iqnp=2 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m4096 -e100000000 -c600.perlbench_s_rand_test.21.gz pk
p93:
	./721sim --perf=1,1,1,1 --fq=64 --cp=64 --al=512 --lsq=256 --iq=128 --iqnp=4 --fw=16 --dw=16 --iw=16 --rw=16 --lane=ffff:ffff:ffff:ffff:ffff:ffff:ffff -m4096 -e100000000 -c600.perlbench_s_rand_test.21.gz pk
clean:
	rm -f *.log *.txt

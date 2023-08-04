all: lib_public.a lib_public.so libftp.a libftp.so

lib_public.a:_public.h _public.cpp
	g++ -c -o lib_public.a _public.cpp

lib_public.so:_public.h _public.cpp
	g++ -fPIC -shared -o lib_public.so _public.cpp

clean:
	rm -f lib_public.a lib_public.so

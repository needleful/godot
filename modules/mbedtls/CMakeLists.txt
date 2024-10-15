project(mbedtls)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

file(GLOB fp_sources *.cpp)
list(APPEND tp_sources
	aes.c
	aesni.c
	arc4.c
	aria.c
	asn1parse.c
	asn1write.c
	base64.c
	bignum.c
	blowfish.c
	camellia.c
	ccm.c
	certs.c
	chacha20.c
	chachapoly.c
	cipher.c
	cipher_wrap.c
	cmac.c
	ctr_drbg.c
	constant_time.c
	debug.c
	des.c
	dhm.c
	ecdh.c
	ecdsa.c
	ecjpake.c
	ecp.c
	ecp_curves.c
	entropy.c
	entropy_poll.c
	error.c
	gcm.c
	havege.c
	hkdf.c
	hmac_drbg.c
	md2.c
	md4.c
	md5.c
	md.c
	memory_buffer_alloc.c
	mps_reader.c
	mps_trace.c
	net_sockets.c
	nist_kw.c
	oid.c
	padlock.c
	pem.c
	pk.c
	pkcs11.c
	pkcs12.c
	pkcs5.c
	pkparse.c
	pk_wrap.c
	pkwrite.c
	platform.c
	platform_util.c
	poly1305.c
	ripemd160.c
	rsa.c
	rsa_internal.c
	sha1.c
	sha256.c
	sha512.c
	ssl_cache.c
	ssl_ciphersuites.c
	ssl_cli.c
	ssl_cookie.c
	ssl_msg.c
	ssl_srv.c
	ssl_ticket.c
	ssl_tls.c
	ssl_tls13_keys.c
	threading.c
	timing.c
	version.c
	version_features.c
	x509.c
	x509_create.c
	x509_crl.c
	x509_crt.c
	x509_csr.c
	x509write_crt.c
	x509write_csr.c
	xtea.c
)

list(TRANSFORM tp_sources PREPEND "${base_dir}/library/")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE "${base_dir}/include/")
#include "url.h"
#include "brucon_nvs.h"
#define BADGE_HOST "badgesvr.brucon.org"
#define WEB_URL    "https://" BADGE_HOST

#define POSTENROLL "/postenroll.php"
#define GETSCHED   "/s/getsched.php"
#define POSTMEASURE "s/c2h6o.php"

#define POSTENROLLURL

char * badgehost = BADGE_HOST;
volatile char hostfound = 0;
ip_addr_t badgesrvip;
struct sockaddr_in server_addr;
struct hostent *server_host;

mbedtls_x509_crt * certchain;
volatile char mbtlsinited = 0;

mbedtls_net_context *server_fd;
mbedtls_entropy_context *entropy;
mbedtls_ctr_drbg_context *ctr_drbg;
mbedtls_ssl_context *ssl;
mbedtls_ssl_config *conf;


#define BRUCON_SSL_VERBOSE
mbedtls_x509_crt * clicert;
mbedtls_pk_context * pk_ctx_clicert;
void disconnect_ssl();
void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
  badgesrvip = *ipaddr;
  hostfound = 1;
  //printf("Server: %s\n", inet_ntoa( *ipaddr ) );
}

int initmbtls()
{
  int ret;
  if(!mbtlsinited){

    server_fd = (mbedtls_net_context*)calloc(1,sizeof(mbedtls_net_context));
    entropy= (mbedtls_entropy_context*)calloc(1,sizeof(mbedtls_entropy_context));
    ctr_drbg= (mbedtls_ctr_drbg_context*)calloc(1,sizeof(mbedtls_ctr_drbg_context));
    ssl= (mbedtls_ssl_context*)calloc(1,sizeof(mbedtls_ssl_context));
    conf= (mbedtls_ssl_config*)calloc(1,sizeof(mbedtls_ssl_config));
    certchain=(mbedtls_x509_crt*)calloc(1,sizeof(mbedtls_x509_crt));
    printf("init ssl context\n");
    mbedtls_x509_crt_init( certchain  );
    //  printf("Cert: %s\n",server_root_cert_pem_start);
    if( (ret=mbedtls_x509_crt_parse(certchain,server_root_cert_pem_start,server_root_cert_pem_end-server_root_cert_pem_start)) != 0){
      printf("Error parsing cert : %d\n",ret);
    }
    //     printf("Cert : %s %s\n",certchain->subject.val.p,certchain->issuer.val.p);
  
  
    mbedtls_entropy_init( entropy );
    mbedtls_ssl_config_init( conf );
    mbedtls_ctr_drbg_init( ctr_drbg );
    mbedtls_ssl_conf_ca_chain( conf, certchain, NULL ); 
    if( ( ret = mbedtls_ctr_drbg_seed( ctr_drbg,
				       //mybsmbedtls_entropy_func,
				       mbedtls_entropy_func,
				       entropy,
				       (uint8_t * ) macstr, //ESP_MAC_WIFI_STA,//(const unsigned char *) pers,
				       strlen( (const char*)macstr) //ESP_MAC_WIFI_STA //pers
				       )) != 0 )
      {
	printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
	return 0;
      }
    if( ( ret = mbedtls_ssl_config_defaults( conf,
					     MBEDTLS_SSL_IS_CLIENT,
					     MBEDTLS_SSL_TRANSPORT_STREAM,
					     MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )	{
      mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
    }
    mbedtls_ssl_conf_authmode( conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_rng( conf, mbedtls_ctr_drbg_random, ctr_drbg );
    if(getBruCONConfigFlag("haveClientCert")!=0){
      printf("Setting clicert %p %p\n",clicert,pk_ctx_clicert);
      if((ret = mbedtls_ssl_conf_own_cert(conf,clicert,pk_ctx_clicert))!=0){
	printf("ERROR own cert :-0x%x\n",ret);
      }
    }
    mbedtls_ssl_conf_renegotiation(conf,MBEDTLS_SSL_RENEGOTIATION_ENABLED );
  
    mbtlsinited = 1;
    return 1;
  }
  return mbtlsinited;
}

	



void solvebadgehost()
{
  if(!hostfound){
    dns_gethostbyname(badgehost, &badgesrvip, dns_found_cb, NULL );
    while(!hostfound);
  }
}


int connectssl()
{
  int ret;
  char buf[512];
  mbedtls_net_init( server_fd );
  mbedtls_ssl_init( ssl );
  if( ( ret = mbedtls_net_connect( server_fd, BADGE_HOST, "443", MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {        printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );    }
   
  if(getBruCONConfigFlag("haveClientCert")!=0){
    // printf("Setting clicert %p %p\n",clicert,pk_ctx_clicert);
    if((ret = mbedtls_ssl_conf_own_cert(conf,clicert,pk_ctx_clicert))!=0){
      printf("ERROR own cert :-0x%x\n",ret);
    }
  }
   
  //   mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );
  if( ( ret = mbedtls_ssl_setup( ssl, conf ) ) != 0 )
    {mbedtls_strerror (ret,buf, 512);
      mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %x\n%s\n", ret,buf );
    }
  if( ( ret = mbedtls_ssl_set_hostname( ssl, BADGE_HOST) ) != 0 )
    {
      mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
    }
  mbedtls_ssl_set_bio( ssl, server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
  while( ( ret = mbedtls_ssl_handshake( ssl ) ) != 0 )
    {
      if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
	  mbedtls_strerror (ret,buf, 512);
	  mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n :%s\n", -ret,buf );
	  //   disconnect_ssl();
	  return ret;

        }
    }
  return ret;
}

int initurl()
{
    solvebadgehost();
    needWifi(true);

    initmbtls();
    return connectssl();
}
void disconnect_ssl()
{

  
    mbedtls_net_free( server_fd );
    mbedtls_ssl_free( ssl );

    /*
    mbedtls_x509_crt_free( certchain );

    mbedtls_ssl_config_free( conf );
    mbedtls_ctr_drbg_free( ctr_drbg );
    mbedtls_entropy_free( entropy );
    free( server_fd );
    free( certchain );
    free( ssl );
    free( conf );
    free( ctr_drbg );
    free( entropy );
    mbtlsinited=0;*/
    

}

#define BUF_SZ 1024
int read_http_response(char ** response, int * response_size,char with_headers)
{
    unsigned char * buf = (unsigned char *)calloc(BUF_SZ,sizeof(char));
    char init = 1;
    char * contentlen=NULL;
    char * ret_str=NULL;
    int curr_sz = 0;  int len;int ret;
    int code;
    do
    {
        len = BUF_SZ - 1;
        memset( buf, 0, BUF_SZ );
        ret = mbedtls_ssl_read( ssl, buf, len );
        if( ret <= 0 )
        {

	  if(ret != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY){
	    printf( "failed\n  ! ssl_read returned -%04x\n\n", -ret );
	    char * errstr = (char*)calloc(200,sizeof(char));
	    memset(errstr, 0, 200 );
	    mbedtls_strerror(ret,errstr,199);
	    printf("%s",errstr);
	    free(errstr);
	  }
            * response = ret_str;
            break;
        }
        else {
            len = ret;
            //printf( "\n%d bytes read\n\n%s\n %d -> %d \n", len, (char *) buf, curr_sz, curr_sz+len);
            // skip http headers
            if(!init ){
                memcpy(ret_str+curr_sz,buf,len);
                curr_sz+=len;
                *(ret_str+curr_sz+1)=0;
            } else {
                // http response headers

	      code=strtoll((char*)buf+9,NULL,10);
	      //	      printf("HTTPCODE:%d %s\n",code,buf);
	      if(code != 200){
	
		*response_size=0;
		*response = NULL;
		break;
		
	      }
	      contentlen=strstr((char*)buf,"Content-Length:");
		
	      if(contentlen != NULL){
		//yey have len
		*response_size = strtoul(contentlen + 15,NULL,10);
		if(with_headers)
		  *response_size += len;
		
		if(*response_size > 100000) // no way not enough ram
		  return -1;
		ret_str = (char*)calloc((*response_size)+3,sizeof(char));
		memset( ret_str, 0, (*response_size)+3);
	      } else {
		//no len no dice, this is IOT !
		return -1;
	      }
	      if(with_headers){
		memcpy(ret_str+curr_sz,buf,len);
		curr_sz+=len;
		*(ret_str+curr_sz+1)=0;
	      }
	      init=0;
            }
        }
    } while( 1 );
    
    free(buf);
    return curr_sz;

}



char * sched_str=NULL;
char * client_cert=NULL;

char * postfile(char * action, char * fieldname,char* filename,char * filecontent)
{
    char * boundary = "BruCONBoundary";
    int i=0;
    int lenc,lenh,len,ret;

    char * c_buff, * h_buff;

    char * formath =
        "POST %s HTTP/1.0\r\n"
        "Host: "BADGE_HOST "\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: multipart/form-data; boundary=%s"
        "\r\n"
        "\r\n";
    char * formatc =
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\""
        "\r\n"
        "Content-Type: application/octet-stream"
        "\r\n"
        "\r\n%s\r\n--%s--\r\n";

    lenc = asprintf(&c_buff,formatc,boundary,fieldname,filename,filecontent,boundary);
    lenh = asprintf(&h_buff,formath,action,lenc,boundary);

    /* printf("%s",h_buff);
       printf("%s\n",c_buff); */
    initurl();
    ret = mbedtls_ssl_write( ssl, (unsigned char*)h_buff, lenh);
    ret = mbedtls_ssl_write( ssl, (unsigned char*)c_buff, lenc);
    ret = read_http_response(&client_cert,&len,0);
    mbedtls_ssl_close_notify( ssl );
    disconnect_ssl();

    free(h_buff);
    free(c_buff);

    return(client_cert);


}

volatile char ispostingAlc;
void send_alc_reading(int32_t * alc){
  unsigned char buf[512];int ret;
  printf("sending score: %d\n",*alc);
  ispostingAlc=1;
  ret = initurl();
  if(ret != 0){
    printf( " ssl init failed\n" );
    disconnect_ssl();
    ispostingAlc=0;
    printf("posting alcool failed, sorry, meh...\n");
    while(1){ vTaskDelay(10000);
    }
  }
  int len = sprintf((char*)buf,"GET " POSTMEASURE "?reading=%d HTTP/1.0\r\nHost: "BADGE_HOST "\r\n\r\n",*alc );
  ret = mbedtls_ssl_write( ssl, buf, len);
  if(ret < 0){
    printf("ss error -%04x = %d , %s\n",-ret,len,buf);
  }
  ret = read_http_response(&buf,&len,1);
  printf("%d %s",len,buf);
  disconnect_ssl();
  ispostingAlc=0;
  
  while(1){ vTaskDelay(10000);
  }
}

volatile char isgettingsched;
char * get_sched()
{
  int ret;
  unsigned char buf[512];
  isgettingsched=1;
  ret = initurl();
  if(ret != 0){
#ifdef BRUCON_SSL_VERBOSE
    printf( " ssl init failed\n" );
#endif
    netsched = NULL; disconnect_ssl();
    isgettingsched=0;
    while(1){ vTaskDelay(10000);
    }

  }
  int len = sprintf((char*)buf,"GET " GETSCHED " HTTP/1.0\r\nHost: "BADGE_HOST "\r\n\r\n" );
  ret = mbedtls_ssl_write( ssl, buf, len);
  if(ret < 0){
    printf("ss error -%04x = %d , %s\n",-ret,len,buf);
  }
  ret = read_http_response(&netsched,&len,0);
  //  printf("\%d %d %d %s\n",ret,len,strlen(sched_str),sched_str);
  mbedtls_ssl_close_notify( ssl );
  disconnect_ssl();
  isgettingsched=0;
  while(1)  {
    vTaskDelay(10000);
  }
  return NULL;
}

int gen_rsakp_and_csr(mbedtls_pk_context * pubkey,unsigned char* buff,uint16_t buffSize)
{
    mbedtls_entropy_context * entropy_dcert;
    mbedtls_ctr_drbg_context * ctr_drbg_dcert;
    mbedtls_rsa_context * rsa_ctx_dcert;
    mbedtls_x509write_csr * req;
    int ret;
    entropy_dcert = (mbedtls_entropy_context*)calloc(1,sizeof(mbedtls_entropy_context));
    ctr_drbg_dcert= (mbedtls_ctr_drbg_context*)calloc(1,sizeof(mbedtls_ctr_drbg_context));
    //rsa_ctx_dcert = (mbedtls_rsa_context*)calloc(1,sizeof(mbedtls_rsa_context));
    req = (mbedtls_x509write_csr*)calloc(1,sizeof(mbedtls_x509write_csr));
    mbedtls_entropy_init( entropy_dcert );
    mbedtls_ctr_drbg_init( ctr_drbg_dcert );
    mbedtls_ctr_drbg_seed( ctr_drbg_dcert, mbedtls_entropy_func, entropy_dcert,(uint8_t * ) macstr,strlen( (const char*)macstr));
    mbedtls_x509write_csr_init(req);
    mbedtls_x509write_csr_set_md_alg( req, MBEDTLS_MD_SHA256 );
    mbedtls_x509write_csr_set_key_usage(req,MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
    mbedtls_x509write_csr_set_ns_cert_type(req,MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT);
    sprintf((char*)buff,"CN=%s",(char*)macstr);
    ret = mbedtls_x509write_csr_set_subject_name(req,(char*)buff);
    if(ret != 0 ){
        printf("error subject_name : 0x%04X, %s\n",-ret,buff);
    } else {
        printf("ok subject_name : %s\n",buff);
    }

    //this IS used in spite of the compiler warning, don't be debian...
    rsa_ctx_dcert =(mbedtls_rsa_context *) pubkey->pk_ctx ;

    mbedtls_rsa_init(rsa_ctx_dcert, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    if(mbedtls_rsa_gen_key(rsa_ctx_dcert, mbedtls_ctr_drbg_random, ctr_drbg_dcert, 1024, 65537) != 0){
        printf("can't gen rsa keypair, abort\n");
        return -1;
    } else {
        printf("\nrsa gen'd\n");
    }
    mbedtls_rsa_complete(rsa_ctx_dcert);

    mbedtls_x509write_csr_set_key( req, pubkey );
    mbedtls_x509write_csr_pem(req,buff,4096,mbedtls_ctr_drbg_random,ctr_drbg_dcert);

    // printf("---\n%s\n---\n",buff);

    mbedtls_x509write_csr_free(req);
    mbedtls_ctr_drbg_free( ctr_drbg_dcert );
    mbedtls_entropy_free( entropy_dcert );



    free(entropy_dcert);
    free(ctr_drbg_dcert);
    free(req);
    return 0;
}

volatile char isgeneratingRSA = 1;

void restore_clicert( 	mbedtls_x509_crt ** cert,mbedtls_pk_context ** pk_ctx_dcert  ){
  char * certstr;
  char * p_str=NULL;
  char * q_str=NULL;
  char * e_str=NULL;
  mbedtls_mpi t,p,q,e;
  if( *cert != NULL)
    return;
  int ret;
  *cert=(mbedtls_x509_crt*)calloc(1,sizeof(mbedtls_x509_crt));
  mbedtls_x509_crt_init( *cert);
  mbedtls_rsa_context * rsa_ctx_dcert;
  mbedtls_pk_context * pubkey;
  pubkey = (mbedtls_pk_context*)calloc(1,sizeof(mbedtls_pk_context));
  mbedtls_pk_setup( pubkey, mbedtls_pk_info_from_type( MBEDTLS_PK_RSA ));
  rsa_ctx_dcert =(mbedtls_rsa_context *) pubkey->pk_ctx ;
  //  mbedtls_rsa_init(rsa_ctx_dcert, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
  
  getBruCONConfigString("ClientCert",&certstr);
  getBruCONConfigString("POWER_HISTORY",&p_str);
  getBruCONConfigString("UNIQUE_ID",&q_str);
  getBruCONConfigString("EVERSION",&e_str);
  mbedtls_mpi_init(&q);
  mbedtls_mpi_init(&p);
  mbedtls_mpi_init(&e);

  mbedtls_mpi_init(&t);  
  mbedtls_mpi_read_string(&t,16,p_str);
  mbedtls_mpi_add_int (&p,&t , 1);
  
  mbedtls_mpi_init(&t);
  mbedtls_mpi_read_string(&t,16,q_str);
  mbedtls_mpi_add_int (&q,&t , 1);

  mbedtls_mpi_init(&t);
  mbedtls_mpi_read_string(&t,16,e_str);
  mbedtls_mpi_add_int (&e,&t , 1);
  *pk_ctx_dcert = pubkey;

  free(p_str);
  free(q_str);
  free(e_str);

  mbedtls_rsa_import(rsa_ctx_dcert,NULL,&p,&q,NULL,&e); 
  mbedtls_rsa_complete(rsa_ctx_dcert);
  //  printf("STRS RSA : \n\t%s\n\t%s\n\t%s",e_str,p_str,q_str);
  //  printf("%s\n",certstr==NULL?"NULL":certstr);
  if( (ret=mbedtls_x509_crt_parse(*cert,(unsigned char*)certstr,strlen(certstr)+1)) != 0){
  printf("Error parsing cli cert : %x\n",-ret);
  }
  free(certstr);
}



void save_rsa(mbedtls_rsa_context * rsa_ctx_dcert)
{
    size_t olen=0;
    size_t max = 0;
    mbedtls_mpi res;
    mbedtls_mpi_init(&res);
    mbedtls_mpi_write_string(&res,16,NULL,0,&olen);
    if(olen > max)
        max=olen;
    // printf("Esz = %d,\n",olen);
    mbedtls_mpi_write_string(&(rsa_ctx_dcert->P),16,NULL,0,&olen);
    if(olen > max)
        max=olen;
    //   printf("Psz = %d,\n",olen);
    mbedtls_mpi_write_string(&(rsa_ctx_dcert->Q),16,NULL,0,&olen);
    //    printf("Qsz = %d,\n",olen);
    if(olen > max)
        max=olen;
//    printf("maxolen = %d\n",max);
    char * savestr = calloc(max,sizeof(char));
    memset( savestr, 0, max );
    mbedtls_mpi_sub_int (&res,&(rsa_ctx_dcert->E) , 1);    
    mbedtls_mpi_write_string(&res,16,savestr,max,&olen);
    setBruCONConfigString("EVERSION",savestr);
    //  printf("E %s\n",savestr);
     
    memset( savestr, 0, max );
    mbedtls_mpi_sub_int (&res,&(rsa_ctx_dcert->P) , 1);    
    mbedtls_mpi_write_string(&res,16,savestr,max,&olen);
    setBruCONConfigString("POWER_HISTORY",savestr);
    //    printf("P %s\n",savestr);
    memset( savestr, 0, max );
    mbedtls_mpi_sub_int (&res,&(rsa_ctx_dcert->Q) , 1);    
    mbedtls_mpi_write_string(&res,16,savestr,max,&olen);
    setBruCONConfigString("UNIQUE_ID",savestr);
    //    printf("Q %s\n",savestr);
}

void save_csr(char * buff)
{
    setBruCONConfigFlag("haveCSR",1);
    setBruCONConfigString("csr",buff);
}


void task_genrsa(char * ret)
{
    uint16_t buffsize = 4096;
    mbedtls_pk_context * pubkey;
    pubkey = (mbedtls_pk_context*)calloc(1,sizeof(mbedtls_pk_context));
    unsigned char * buff = (unsigned char*)calloc(buffsize,sizeof(char));
    memset( buff, 0,buffsize );
    mbedtls_pk_init(pubkey );
    mbedtls_pk_setup( pubkey, mbedtls_pk_info_from_type( MBEDTLS_PK_RSA ));
    mbedtls_rsa_context * rsa_ctx_dcert;
    gen_rsakp_and_csr(pubkey,buff,buffsize);
    rsa_ctx_dcert = (mbedtls_rsa_context *) pubkey->pk_ctx;


  

    save_rsa( rsa_ctx_dcert);
    setBruCONConfigFlag("haveKeys",1);
    save_csr((char*)buff);
    setBruCONConfigFlag("haveCSR",1);


    mbedtls_pk_free(pubkey);
    free(pubkey);
    free(buff);
    isgeneratingRSA=0;
    //  printf("OUT RSA\n");

    while(1)  {
        vTaskDelay(10000);
    }



}

volatile char isgettingClientCert;
void task_getclientcert(char * ret)
{
    char * csr;
    getBruCONConfigString("csr",&csr);
    postfile("/enroll.php","CSR","CSR",csr);
    //printf("CLICERT:\n%s\n",client_cert);
    free(csr);
    BruCONErase_key("csr");
    setBruCONConfigFlag("haveClientCert",1);
    setBruCONConfigString("ClientCert",client_cert);
    //  printf("%s",client_cert);
    setBruCONConfigFlag("haveCSR",0);

   
    
    isgettingClientCert=0;
    while(1)  {
        vTaskDelay(10000);
    }
}

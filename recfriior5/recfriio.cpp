// $Id: recfriio.cpp 5710 2008-09-19 15:04:04Z clworld $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <signal.h>
#include <fcntl.h>

#include <inttypes.h>
#include <dirent.h>
#include <errno.h>

#include <math.h>

#include <err.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#ifdef HTTP
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif /* defined(HTTP) */

/* maximum write length at once */
#define SIZE_CHUNK 1316

#include <boost/scoped_ptr.hpp>

#include "setting.hpp"
#include "error.hpp"
#include "Recordable.hpp"
#ifdef B25
#include "B25Decoder.hpp"
#endif /* defined(B25) */
#ifdef UDP
#include "Udp.hpp"
#endif /* defined(UDP) */
#ifdef TSSL
#include "tssplitter_lite.hpp"
#endif /* defined(TSSL) */

/**
 * usageの表示
 */
void usage(char *argv[])
{
	std::cerr << "usage: " << argv[0]
#ifdef B25
		<< " [--b25 [--round N] [--strip] [--EMM] [--sync]]"
#endif /* defined(B25) */
#ifdef HDUS
		<< " [--hdus]"
		<< " [--hdp]"
#endif /* defined(HDUS) */
		<< " [--lockfile lock]"
		<< " [--lnb]"
#ifdef UDP
		<< " [--udp ip [--port N]]"
#endif /* defined(UDP) */
#ifdef HTTP
		<< " [--http PortNo]"
#endif /* defined(HTTP) */
#ifdef TSSL
		<< " [--sid SID1,SID2]"
#endif /* defined(TSSL) */
		<< " channel recsec destfile" << std::endl;
	std::cerr << "Channels:" << std::endl;
	std::cerr << "  13 - 62   : UHF13 - UHF62" << std::endl;
#ifdef HDUS
	std::cerr << "  K13 - K63 : CATV13 - CATV63" << std::endl;
#endif /* defined(HDUS) */
	std::cerr << "  B1 ,BS01_0: BS朝日               C1 ,CS2 : ND2  110CS" << std::endl;
	std::cerr << "  B2 ,BS01_1: BS-TBS               C2 ,CS4 : ND4  110CS" << std::endl;
	std::cerr << "  B3 ,BS03_0: WOWOWプライム        C3 ,CS6 : ND6  110CS" << std::endl;
	std::cerr << "  B4 ,BS03_1: BSジャパン           C4 ,CS8 : ND8  110CS" << std::endl;
	std::cerr << "  B5 ,BS05_0: WOWOWライブ          C5 ,CS10: ND10 110CS" << std::endl;
	std::cerr << "  B6 ,BS05_1: WOWOWシネマ          C6 ,CS12: ND12 110CS" << std::endl;
	std::cerr << "  B7 ,BS07_0: スターチャンネル2/3  C7 ,CS14: ND14 110CS" << std::endl;
	std::cerr << "  B8 ,BS07_1: BSアニマックス       C8 ,CS16: ND16 110CS" << std::endl;
	std::cerr << "  B9 ,BS07_2: ディズニーチャンネル C9 ,CS18: ND18 110CS" << std::endl;
	std::cerr << "  B10,BS09_0: BS11                 C10,CS20: ND20 110CS" << std::endl;
	std::cerr << "  B11,BS09_1: スターチャンネル1    C11,CS22: ND22 110CS" << std::endl;
	std::cerr << "  B12,BS09_2: TwellV               C12,CS24: ND24 110CS" << std::endl;
	std::cerr << "  B13,BS11_0: FOX bs238" << std::endl;
	std::cerr << "  B14,BS11_1: BSスカパー!" << std::endl;
	std::cerr << "  B15,BS11_2: 放送大学" << std::endl;
	std::cerr << "  B16,BS13_0: BS日テレ" << std::endl;
	std::cerr << "  B17,BS13_1: BSフジ" << std::endl;
	std::cerr << "  B18,BS15_0: NHK BS1" << std::endl;
	std::cerr << "  B19,BS15_1: NHK BSプレミアム" << std::endl;
	std::cerr << "  B20,BS17_0: 地デジ難視聴1(NHK/NHK-E/CX)" << std::endl;
	std::cerr << "  B21,BS17_1: 地デジ難視聴2(NTV/TBS/EX/TX)" << std::endl;
	std::cerr << "  B22,BS19_0: グリーンチャンネル" << std::endl;
	std::cerr << "  B23,BS19_1: J SPORTS 1" << std::endl;
	std::cerr << "  B24,BS19_2: J SPORTS 2" << std::endl;
	std::cerr << "  B25,BS21_0: IMAGICA BS" << std::endl;
	std::cerr << "  B26,BS21_1: J SPORTS 3" << std::endl;
	std::cerr << "  B27,BS21_2: J SPORTS 4" << std::endl;
	std::cerr << "  B28,BS23_0: BS釣りビジョン" << std::endl;
	std::cerr << "  B29,BS23_1: 日本映画専門チャンネル" << std::endl;
	std::cerr << "  B30,BS23_2: D-Life" << std::endl;
	exit(1);
}

/**
 * オプション情報
 */
struct Args {
#ifdef B25
	bool b25;
	int round;
	bool strip;
	bool emm;
	bool sync;
#endif /* defined(B25) */
#ifdef HDUS
	bool use_hdus;
	bool use_hdp;
#endif /* defined(HDUS) */
	char* lockfile;
	bool lnb;
#ifdef UDP
	std::string ip;
	int port;
#endif /* defined(UDP) */
#ifdef HTTP
	bool http_mode;
	int http_port;
#endif /* defined(HTTP) */
#ifdef TSSL
	bool splitter;
	char *sid_list;
#endif /* defined(TSSL) */
	bool stdout;
	TunerType type;
	BandType band;
	int channel;
	bool forever;
	int recsec;
	char* destfile;
};

/**
 * チャンネルの解析
 */
void
parseChannel(Args* args, char* chstr)
{
	int channel = 0;
	switch (chstr[0]) {
		case 'B':
		case 'b':
			if (chstr[1] == 'S' || chstr[1] == 's') {
				int solt = 8;
				args->type = TUNER_FRIIO_BLACK;
				args->band = BAND_BS;
				chstr += 2;
				while (isdigit( *chstr ))
					channel = channel * 10 + ( *chstr++ - '0' );
				if (channel == 0 || channel > 23 || ( channel & 0x01 ) == 0 || *chstr != '_') {
					std::cerr << "BS channel node must be (1 <= node <= 23)." << std::endl;
					exit(1);
				}
				if (isdigit( *++chstr ))
					solt = *chstr++ - '0';
				if (solt >= 8 || *chstr != '\0') {
					std::cerr << "BS channel solt must be (0 <= solt <= 7)." << std::endl;
					exit(1);
				}
				// arib仕様(旧機器対策)
				if (channel == 15)
					solt++;
				// TSID合成(登録年情報欠落)
				channel = 0x4000 | (channel << 4) | solt;
			} else {
				args->type = TUNER_FRIIO_BLACK;
				args->band = BAND_BS;
				chstr++;
				channel = atoi(chstr);
				if (channel < 1 || 30 < channel) {
					std::cerr << "channel must be (B1 <= channel <= B30)." << std::endl;
					exit(1);
				}
			}
			break;
		case 'C':
		case 'c':
			if (chstr[1] == 'S' || chstr[1] == 's') {
				args->type = TUNER_FRIIO_BLACK;
				args->band = BAND_CS;
				chstr += 2;
				channel = atoi(chstr);
				if ((channel & 0x01) == 0) {
					channel /= 2;
					if (1 <= channel && channel <= 12)
						break;
				}
				std::cerr << "channel must be (CS2 <= channel <= CS24)." << std::endl;
				exit(1);
			} else {
				args->type = TUNER_FRIIO_BLACK;
				args->band = BAND_CS;
				chstr++;
				channel = atoi(chstr);
				if (channel < 1 || 12 < channel) {
					std::cerr << "channel must be (C1 <= channel <= C12)." << std::endl;
					exit(1);
				}
			}
			break;
#ifdef HDUS
		case 'K':
		case 'k':
			if( args->use_hdus )
				args->type = TUNER_HDUS;
			else
				args->type = TUNER_HDP;
			args->band = BAND_CATV;
			chstr++;
			channel = atoi(chstr);
			if (channel < 13 || 63 < channel) {
				std::cerr << "channel must be (K13 <= channel <= K63)." << std::endl;
				exit(1);
			}
			break;
#endif 
		default:
#ifdef HDUS
			if( args->use_hdus )
				args->type = TUNER_HDUS;
			else if( args->use_hdp )
				args->type = TUNER_HDP;
			else
				args->type = TUNER_FRIIO_WHITE;
#else
			args->type = TUNER_FRIIO_WHITE;
#endif /* defined(HDUS) */
			args->band = BAND_UHF;
			channel = atoi(chstr);
			if (channel < 13 || 62 < channel) {
				std::cerr << "channel must be (13 <= channel <= 62)." << std::endl;
				exit(1);
			}
	}
	args->channel = channel;
}

/**
 * オプションの解析
 */
Args
parseOption(int argc, char *argv[])
{
	Args args = {
#ifdef B25
		false,
		4,
		false,
		false,
		false,
#endif /* defined(B25) */
#ifdef HDUS
		false,
		false,
#endif /* defined(HDUS) */
		NULL,
		false,
#ifdef UDP
		"",
		UDP_PORT,
#endif /* defined(UDP) */
#ifdef HTTP
		false,
		HTTP_PORT,
#endif /* defined(HTTP) */
#ifdef TSSL
		false,
		NULL,
#endif /* defined(TSSL) */
		false,
		TUNER_FRIIO_WHITE,
		BAND_UHF,
		0,
		false,
		0,
		NULL,
	};

	while (1) {
		int option_index = 0;
		static option long_options[] = {
#ifdef B25
			{ "b25",      0, NULL, 'b' },
			{ "B25",      0, NULL, 'b' },
			{ "round",    1, NULL, 'r' },
			{ "strip",    0, NULL, 's' },
			{ "EMM",      0, NULL, 'm' },
			{ "emm",      0, NULL, 'm' },
			{ "sync",     0, NULL, 'S' },
#endif /* defined(B25) */
#ifdef HDUS
			{ "hdus",     0, NULL, 'h' },
			{ "hdp",      0, NULL, 'd' },
#endif /* defined(HDUS) */
			{ "lockfile", 1, NULL, 'l' },
			{ "lnb",      0, NULL, 'n' },
#ifdef UDP
			{ "udp",      1, NULL, 'u' },
			{ "port",     1, NULL, 'p' },
#endif /* defined(UDP) */
#ifdef HTTP
			{ "http",     1, NULL, 'H' },
#endif /* defined(HTTP) */
#ifdef TSSL
			{ "sid",      1, NULL, 'i' },
#endif /* defined(TSSL) */
			{ 0,          0, NULL, 0   }
		};
		
		int r = getopt_long(argc, argv,
#ifdef B25
		                    "br:smS"
#endif /* defined(B25) */
#ifdef UDP
		                    "u:p:"
#endif /* defined(B25) */
#ifdef HTTP
		                    "H:"
#endif /* defined(HTTP) */
#ifdef TSSL
		                    "i:"
#endif /* defined(TSSL) */
		                    "l:",
		                    long_options, &option_index);
		if (r < 0) {
			break;
		}
		
		switch (r) {
#ifdef B25
			case 'b':
				args.b25 = true;
				break;
			case 'r':
				args.round = atoi(optarg);
				break;
			case 's':
				args.strip = true;
				break;
			case 'm':
				args.emm = true;
				break;
			case 'S':
				args.sync = true;
				break;
#endif /* defined(B25) */
#ifdef HDUS
			case 'h':
				args.use_hdus = true;
				break;
			case 'd':
				args.use_hdp = true;
				break;
#endif /* defined(HDUS) */
			case 'l':
				args.lockfile = optarg;
				break;
			case 'n':
				args.lnb = true;
				break;
#ifdef UDP
			case 'u':
				args.ip = optarg;
				break;
			case 'p':
				args.port = atoi(optarg);
				break;
#endif /* defined(UDP) */
#ifdef HTTP
			case 'H':
				args.http_mode = true;
				args.http_port = atoi(optarg);
				args.forever = true;
				return args;
				break;
#endif /* defined(HTTP) */
#ifdef TSSL
			case 'i':
				args.splitter = true;
				args.sid_list = optarg;
				break;
#endif /* defined(TSSL) */
			default:
				break;
		}
	}
	
	if (argc - optind != 3) {
		usage(argv);
	}
	
	parseChannel(&args, argv[optind++]);
	char *recsecstr = argv[optind++];
	if (strcmp("-", recsecstr) == 0) {
		args.forever = true;
	}
	args.recsec    = atoi(recsecstr);
	args.destfile = argv[optind++];
	if (strcmp("-", args.destfile) == 0) {
		args.stdout = true;
	}
	
	return args;
}

#ifdef HTTP
//read 1st line from socket
int read_line(int socket, char *p){
	int len = 0;
	while (1){
		int ret;
		ret = read(socket, p, 1);
		if ( ret == -1 ){
			perror("read");
			exit(1);
		} else if ( ret == 0 ){
			break;
		}
		if ( *p == '\n' ){
			p++;
			len++;
			break;
		}
		p++;
		len++;
	}
	*p = '\0';
	return len;
}
#endif /* defined(HTTP) */

/** シグナルを受けた */
static bool caughtSignal = false;

/** シグナルハンドラ */
void
sighandler(int arg)
{
	caughtSignal = true;
}

/** main */
int
main(int argc, char *argv[])
{
	Args args = parseOption(argc, argv);
	// 正常終了時戻り値
	int result = 0;
	boost::scoped_ptr<Recordable> tuner(NULL);
	timeval tv_start;
#ifdef UDP
	Udp udp;
#endif /* defined(UDP) */
#ifdef HTTP
	int dest = 1; // stdout
	int connected_socket = 0;
	int listening_socket = 0;
#endif /* defined(HTTP) */
	
	// 引数確認
	if (!args.forever && args.recsec <= 0) {
		std::cerr << "recsec must be (recsec > 0)." << std::endl;
		exit(1);
	}
	
	// 録画時間の基準開始時間
	time_t time_start = time(NULL);
	
	// ログ出力先設定
	std::ostream& log = args.stdout ? std::cerr : std::cout;

#ifdef HTTP
	if( !args.http_mode ){
		// 出力先ファイルオープン
		if(!args.stdout) {
			dest = open(args.destfile, (O_RDWR | O_CREAT | O_TRUNC), 0666);
			if (0 > dest) {
				std::cerr << "can't open file '" << args.destfile << "' to write." << std::endl;
				exit(1);
			}
		}
	}else{
		struct sockaddr_in	sin;
		int					sock_optval = 1;
		int					ret;

		fprintf(stderr, "run as a daemon..\n");
		if(daemon(1,1)){
			perror("failed to start");
			exit(1);
		}

		listening_socket = socket(AF_INET, SOCK_STREAM, 0);
		if ( listening_socket == -1 ){
			perror("socket");
			exit(1);
		}

		if ( setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &sock_optval, sizeof(sock_optval)) == -1 ){
			perror("setsockopt");
			exit(1);
		}

		sin.sin_family = AF_INET;
		sin.sin_port = htons(args.http_port);
		sin.sin_addr.s_addr = htonl(INADDR_ANY);

		if ( bind(listening_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0 ){
			perror("bind");
			exit(1);
		}

		ret = listen(listening_socket, SOMAXCONN);
		if ( ret == -1 ){
			perror("listen");
			exit(1);
		}
		fprintf(stderr,"listening at port %d\n", args.http_port);
	}

	while(1){
		if ( args.http_mode ) {
			struct sockaddr_in	peer_sin;
			int					read_size;
			unsigned int		len;
			char				buffer[256];
			char				s0[256],s1[256],s2[256];
			char				delim[] = "/";
			char				*channel;
			char				*sidflg;

			len = sizeof(peer_sin);
			connected_socket = accept(listening_socket, (struct sockaddr *)&peer_sin, &len);
			if ( connected_socket == -1 ) {
				perror("accept");
				exit(1);
			}

			int error;
			char hbuf[NI_MAXHOST], nhbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
			error = getnameinfo((struct sockaddr *)&peer_sin, sizeof(peer_sin), hbuf, sizeof(hbuf), NULL, 0, 0);
			if (error) {
				fprintf(stderr, "getnameinfo(): %s\n", gai_strerror(error));
				exit(1);
			}
			error = getnameinfo((struct sockaddr *)&peer_sin, sizeof(peer_sin), nhbuf, sizeof(nhbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
			if (error) {
				fprintf(stderr, "getnameinfo(): %s\n", gai_strerror(error));
				exit(1);
			}
			fprintf(stderr,"connect from: %s [%s] port %s\n", hbuf, nhbuf, sbuf);

			read_size = read_line(connected_socket, buffer);
			fprintf(stderr, "request command is %s\n", buffer);
			// ex:GET /C8/333 HTTP/1.1
			sscanf(buffer, "%s%s%s", s0, s1, s2);
			channel = strtok(s1, delim);
			if (channel != NULL) {
				fprintf(stderr, "Channel: %s\n", channel);
				parseChannel(&args, channel);
				sidflg = strtok(NULL, delim);
				if (sidflg != NULL) {
					fprintf(stderr, "SID: %s\n", sidflg);
#ifdef TSSL
					args.splitter = true;
					args.sid_list = sidflg;
				} else {
					args.splitter = false;
					args.sid_list = NULL;
#endif /* defined(TSSL) */
				}
			}
			char header[] =  "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nCache-Control: no-cache\r\n\r\n";
			write(connected_socket, header, strlen(header));
			//set write target to http
			dest = connected_socket;
		}
#endif /* defined(HTTP) */

#ifdef B25
	// B25初期化
	B25Decoder b25dec;
	if (args.b25) {
		try {
			b25dec.setRound(args.round);
			b25dec.setStrip(args.strip);
			b25dec.setEmmProcess(args.emm);
			b25dec.open();
			log << "B25Decoder initialized." << std::endl;
		} catch (b25_error& e) {
			std::cerr << e.what() << std::endl;
			
#ifdef HTTP
			if (!args.http_mode) {
#endif /* defined(HTTP) */
			// エラー時b25を行わず処理続行。終了ステータス1
			std::cerr << "disable b25 decoding." << std::endl;
			args.b25 = false;
			result = 1;
#ifdef HTTP
			}
#endif /* defined(HTTP) */
		}
	}
#endif /* defined(B25) */

#ifdef UDP
	// UDP初期化
	if( ! args.ip.empty() ){
		try{
			udp.setLog(&log);
			udp.init( args.ip, args.port );
		}
		catch( const char* e ){
			log << e << std::endl;
			log << "disable UDP." << std::endl;
		}
	}
#endif /* defined(UDP) */

#ifdef TSSL
	/* initialize splitter */
	splitbuf_t splitbuf;
	splitbuf.size = 0;
	splitter *splitter = NULL;
	int split_select_finish = TSS_ERROR;
	int code;
	if(args.splitter) {
		splitter = split_startup(args.sid_list);
		if(splitter->sid_list == NULL) {
			fprintf(stderr, "Cannot start TS splitter\n");
			return 1;
		}
	}
#endif /* defined(TSSL) */

	// Tuner取得
	tuner.reset(createRecordable(args.type));
#ifdef HDUS
	if( args.type == TUNER_HDUS ) log << "Tuner type is HDUS." << std::endl;
	else if( args.type == TUNER_HDP ) log << "Tuner type is HDP." << std::endl;
#endif /* defined(HDUS) */
	// ログ出力先設定
	tuner->setLog(&log);
	// ロックファイル設定
	if (args.lockfile != NULL) {
		tuner->setDetectLockFile(args.lockfile);
	}
	
	// Tuner初期化
	int retryCount = ERROR_RETRY_MAX;
	while (0 < retryCount) {
		try {
			// チューナopen
			bool r = tuner->open(args.lnb);
			if (!r) {
				std::cerr << "can't open tuner." << std::endl;
				exit(1);
			}
			
			// チャンネル設定
			tuner->setChannel(args.band, args.channel);
			
			// 開始時SignalLevel出力
			float lev_before = 0.0;
			int lev_retry_count = SIGNALLEVEL_RETRY_MAX;
			while (lev_before < SIGNALLEVEL_RETRY_THRESHOLD && 0 < lev_retry_count) {
				lev_before = tuner->getSignalLevel();
				log << "Signal level: " << lev_before << std::endl;
				
				lev_retry_count--;
				usleep(SIGNALLEVEL_RETRY_INTERVAL * 1000);
			}
		} catch (usb_error& e) {
			// リトライ処理
			retryCount--;
			std::cerr << e.what();
			if (retryCount <= 0) {
				std::cerr << " abort." << std::endl;
				exit(1);
			}
			std::cerr << " retry." << std::endl;
			
			tuner->close();
			usleep(ERROR_RETRY_INTERVAL * 1000);
			continue;
		}
		break;
	}
	
#ifndef HTTP
	// 出力先ファイルオープン
	FILE *dest = stdout;
	if (!args.stdout) {
		dest = fopen(args.destfile, "w");
		if (NULL == dest) {
			std::cerr << "can't open file '" << args.destfile << "' to write." << std::endl;
			exit(1);
		}
	}
#endif /* !defined(HTTP) */
	
	// 出力開始/時間計測
	log << "Output ts file." << std::endl;
	if (gettimeofday(&tv_start, NULL) < 0) {
		std::cerr << "gettimeofday failed." << std::endl;
		exit(1);
	}
	
	// SIGINT/SIGTERMキャッチ
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	struct sigaction saDefault;
	memset(&saDefault, 0, sizeof(struct sigaction));
	saDefault.sa_handler = SIG_DFL;
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	
	uint8_t		*buf = NULL;
	int			rlen;
	
	// 受信スレッド起動
	tuner->startStream();
	// データ読み出し
	uint32_t urb_error_cnt = 0;
	uint32_t b25_error_cnt = 0;
	while (!caughtSignal && (args.forever || time(NULL) <= time_start + args.recsec)) {
		try {
			rlen = tuner->getStream((const uint8_t **)&buf, 200);
			if (0 == rlen) {
				continue;
			}
			
#ifdef B25
			// B25を経由させる。
			if (args.b25) {
				static int f_b25_sync = 0;
				try {
					uint8_t *b25buf;
					b25dec.put(buf, rlen);
					rlen = b25dec.get((const uint8_t **)&b25buf);
					if (0 == rlen) {
						continue;
					}
					f_b25_sync = 1;
					buf = b25buf;
				} catch (b25_error& e) {
					if( f_b25_sync == 0 && args.sync ){
						log << "Wait for B25 sync" << std::endl;
						continue;
					}
			
#ifdef HTTP
					if (!args.http_mode) {
#endif /* defined(HTTP) */
					// b25停止、戻り値エラー
					throw;
#ifdef HTTP
					}
#endif /* defined(HTTP) */
				}
			}
#endif /* defined(B25) */

#ifdef TSSL
			if (args.splitter) {
				splitbuf.size = 0;
				while (rlen) {
					/* 分離対象PIDの抽出 */
					if (split_select_finish != TSS_SUCCESS) {
						split_select_finish = split_select(splitter, buf, rlen);
						if (split_select_finish == TSS_NULL) {
							/* mallocエラー発生 */
							log << "split_select malloc failed" << std::endl;
							args.splitter = false;
							result = 1;
							goto fin;
						} else if (split_select_finish != TSS_SUCCESS) {
							// 分離対象PIDが完全に抽出できるまで出力しない
							// 1秒程度余裕を見るといいかも
							time_t cur_time;
							time(&cur_time);
							if (cur_time - time_start > 4) {
								args.splitter = false;
								result = 1;
								goto fin;
							}
							break;
						}
					}
					/* 分離対象以外をふるい落とす */
					code = split_ts(splitter, buf, rlen, &splitbuf);
					if (code != TSS_SUCCESS) {
						log << "split_ts failed" << std::endl;
						break;
					}
					break;
				}

				rlen = splitbuf.size;
				buf = splitbuf.buffer;
			fin:
				;
			}
#endif /* defined(TSSL) */

#ifdef UDP
			// UDP 配信
			udp.send(buf, rlen);
#endif /* defined(UDP) */

#ifdef HTTP
			while(rlen > 0) {
				ssize_t wc;
				int ws = rlen < SIZE_CHUNK ? rlen : SIZE_CHUNK;
				while(ws > 0) {
					wc = write(dest, buf, ws);
					if(wc < 0) {
						log << "write failed." << std::endl;
						rlen = 0;
						buf = NULL;
						break;
					}
					ws -= wc;
					rlen -= wc;
					buf += wc;
				}
			}
#else
			fwrite(buf, 1, rlen, dest);
#endif /* defined(HTTP) */

		} catch (usb_error& e) {
			if (urb_error_cnt <= URB_ERROR_MAX) {
				log << e.what() << std::endl;
				if (urb_error_cnt == URB_ERROR_MAX) {
					log << "Too many URB error." << std::endl;
				}
				urb_error_cnt++;
			}
		} catch (b25_error& e) {
			if (b25_error_cnt <= B25_ERROR_MAX) {
				if(b25_error_cnt == B25_ERROR_MAX){
					log << "B25 Error: " << e.what() << std::endl;
					log << "Continue recording without B25." << std::endl;
					args.b25 = false;
					result = 1;
				}
				b25_error_cnt++;
			}
		}
	}
	if (caughtSignal) {
#ifdef HTTP
		if( args.http_mode )
			caughtSignal = false;
		else
#endif /* defined(HTTP) */
		log << "interrupted." << std::endl;
	}
	// 受信スレッド停止
	tuner->stopStream();
	
	// シグナルハンドラを戻す。
	sigaction(SIGINT,  &saDefault, NULL);
	sigaction(SIGTERM, &saDefault, NULL);
	sigaction(SIGPIPE, &saDefault, NULL);

	rlen = 0;
	buf = NULL;

#ifdef B25
	// B25デコーダ内のデータを出力する。
	if (args.b25) {
		try {
			b25dec.flush();
			rlen = b25dec.get((const uint8_t **)&buf);
		} catch (b25_error& e) {
			log << "B25 Error: " << e.what() << std::endl;
			result = 1;
		}
	}
#endif /* defined(B25) */
#ifdef TSSL
	if (args.splitter) {
		splitbuf.size = 0;
		while (rlen) {
			/* 分離対象PIDの抽出 */
			if (split_select_finish != TSS_SUCCESS) {
				split_select_finish = split_select(splitter, buf, rlen);
				if (split_select_finish == TSS_NULL) {
					/* mallocエラー発生 */
					log << "split_select malloc failed" << std::endl;
					args.splitter = false;
					result = 1;
					break;
				} else if (split_select_finish != TSS_SUCCESS) {
					// 分離対象PIDが完全に抽出できるまで出力しない
					// 1秒程度余裕を見るといいかも
					time_t cur_time;
					time(&cur_time);
					if (cur_time - time_start > 4) {
						args.splitter = false;
						result = 1;
					}
					break;
				}
			}
			/* 分離対象以外をふるい落とす */
			code = split_ts(splitter, buf, rlen, &splitbuf);
			if (code != TSS_SUCCESS) {
				log << "split_ts failed" << std::endl;
				break;
			}
			break;
		}
		rlen = splitbuf.size;
		buf = splitbuf.buffer;
		split_shutdown(splitter);
	}
#endif /* defined(TSSL) */
#ifdef HTTP
		while(rlen > 0) {
			ssize_t wc;
			int ws = rlen < SIZE_CHUNK ? rlen : SIZE_CHUNK;
			while(ws > 0) {
				wc = write(dest, buf, ws);
				if(wc < 0) {
					log << "write failed." << std::endl;
					rlen = 0;
					buf = NULL;
					break;
				}
				ws -= wc;
				rlen -= wc;
				buf += wc;
			}
		}
		if( args.http_mode ){
			/* close http socket */
			close(dest);
			fprintf(stderr,"connection closed. still listening at port %d\n", args.http_port);
		}else
			break;
	}
#else
	if (0 < rlen) {
		fwrite(buf, 1, rlen, dest);
	}
#endif /* defined(HTTP) */

	// 時間計測
	timeval tv_end;
	if (gettimeofday(&tv_end, NULL) < 0) {
		err(1, "gettimeofday failed.");
	}
	
	// 出力先ファイルクローズ
#ifdef HTTP
	if (!args.stdout) {
		close(dest);
	}
#else
	fflush(dest);
	if (!args.stdout) {
		fclose(dest);
	}
#endif /* defined(HTTP) */
	log << "done." << std::endl;
	
#ifdef UDP
	// UDP クローズ
	udp.shutdown();
#endif /* defined(UDP) */
	
	// 録画時間出力
	timeval rec_time;
	timersub(&tv_end, &tv_start, &rec_time);
	log << "Rec time: " << rec_time.tv_sec << "." << std::setfill('0') << std::setw(6) << rec_time.tv_usec << " sec." << std::endl;
	
	// 終了時SignalLevel出力
	try {
		float lev_after = tuner->getSignalLevel();
		log << "Signal level: " << lev_after << std::endl;
	} catch (usb_error& e) {
		log << e.what() << " ignored." << std::endl;
	}
	
	return result;
}

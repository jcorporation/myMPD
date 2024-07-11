PORTNAME=	myMPD
DISTVERSIONPREFIX= v
DISTVERSION=	16.1.0
CATEGORIES=	multimedia

MAINTAINER=	robert.david@posteo.net
COMMENT=	Standalone and mobile friendly web-based MPD client
WWW=		https://github.com/jcorporation/myMPD

LICENSE=	GPLv3+

BUILD_DEPENDS=	jq:textproc/jq
LIB_DEPENDS=	libpcre2-8.so:devel/pcre2 \
		libid3tag.so:audio/libid3tag \
		libFLAC.so:audio/flac \
		libogg.so:audio/libogg \
		liblua-5.4.so:lang/lua54

USES=		cmake:noninja perl5
USE_PERL5=	build
USE_GITHUB=	yes
GH_ACCOUNT=	jcorporation

.include <bsd.port.mk>

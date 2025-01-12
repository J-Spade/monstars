from django.conf import settings
from django.conf.urls.static import static
from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import include, path
from django.views.generic import RedirectView

from . import views

# automatically "register" app URLs here if installed
try:
    import bang
    BANG_URLS = include("bang.urls")
except ImportError:
    BANG_URLS = RedirectView.as_view(url="/")
try:
    import blanko
    BLANKO_URLS = include("blanko.urls")
except ImportError:
    BLANKO_URLS = RedirectView.as_view(url="/")
try:
    import bupkus
    BUPKUS_URLS = include("bupkus.urls")
except ImportError:
    BUPKUS_URLS = RedirectView.as_view(url="/")
try:
    import pound
    NAWT_URLS = include("pound.urls")
except ImportError:
    NAWT_URLS = RedirectView.as_view(url="/")
try:
    import nawt
    POUND_URLS = include("nawt.urls")
except ImportError:
    POUND_URLS = RedirectView.as_view(url="/")

urlpatterns = [
    path("", views.index, name="index"),
    path("signin/", views.signin, name="signin"),
    path("signout/", views.signout, name="signout"),
    path("jam/", views.jam, name="jam"),
    # players
    path("bang/", BANG_URLS, name="bang"),
    path("blanko/", BLANKO_URLS, name="blanko"),
    path("bupkus/", BUPKUS_URLS, name="bupkus"),
    path("nawt/", NAWT_URLS, name="nawt"),
    path("pound/", POUND_URLS, name="pound"),
    # django builtins
    path("admin/", admin.site.urls, name="admin"),
]

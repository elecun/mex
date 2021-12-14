from django.shortcuts import render
from django.contrib.auth.decorators import login_required
from django.contrib.auth import authenticate
# from django.contrib.auth import login as auth_login
# from django.contrib.auth import logout as auth_logout
from django.shortcuts import redirect


# @login_required
def index(request):
    return render(request, "index.html")

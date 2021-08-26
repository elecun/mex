from django.shortcuts import render
from django.contrib.auth.decorators import login_required
from django.contrib.auth import authenticate
# from django.contrib.auth import login as auth_login
# from django.contrib.auth import logout as auth_logout
from django.shortcuts import redirect


# @login_required
def index(request):
    return render(request, "index.html")


# def login(request):
#     if request.method == "POST":
#         _username = request.POST.get('username','')
#         _password = request.POST.get('password','')
#         user = authenticate(request, username=_username, password=_password)

#         if user is not None:
#             auth_login(request, user)
#             return redirect('/')
    
#     return render(request, "login.html")


# @login_required
# def logout(request):
#     auth_logout(request)
#     return redirect('/')
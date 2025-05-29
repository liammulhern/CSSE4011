from django.urls import path
from accounts.views import current_user

urlpatterns = [
    path('user_self/', current_user, name='current_user'),
]

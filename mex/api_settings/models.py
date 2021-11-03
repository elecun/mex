#-*- coding:utf-8 -*-

from django.db import models
import uuid


'''
 Command model (defined with fixtures)
'''
class Command(models.Model):
    command = models.CharField(max_length=16, blank=False, null=False, default="STOP")
    info = models.CharField(max_length=255, blank=True, default="")
    
    def __str__(self):
        return self.command

'''
Settings parameter model
'''
class Settings(models.Model):
    uid = models.CharField(max_length=64, blank=False, null=False, default=uuid.uuid4().hex, unique=True)
    name = models.CharField(max_length=255, blank=False, default="")

    machine_name = models.CharField(max_length=255, blank=True, default="") #장비명
    jsno = models.CharField(max_length=255, blank=True, default="") #JS No
    product_size = models.FloatField(blank=True, null=True, default=0.0) #제품 사이즈
    vload = models.FloatField(blank=True, null=True, default=0.0) #수직하중
    roller_size = models.FloatField(blank=True, null=True, default=0.0) #구동롤러 사이즈
    experiment_time = models.FloatField(blank=True, null=True, default=0.0) #시험시간
    update_period = models.PositiveIntegerField(blank=True, null=True, default=10) # 측정주기
    limit_temperature_min = models.IntegerField(blank=True, null=True, default=0) #제한온도
    limit_temperature_max = models.IntegerField(blank=True, null=True, default=100)
    limit_temperature_min_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_rpm = models.PositiveIntegerField(blank=True, null=True, default=0) #제한속도
    limit_rpm_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_load_min = models.FloatField(blank=True, null=True, default=0.0) #제한하중
    limit_load_max = models.FloatField(blank=True, null=True, default=0.0)
    limit_load_min_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_load_max_count = models.PositiveIntegerField(blank=True, null=True, default=3)

    def __str__(self):
        return self.name
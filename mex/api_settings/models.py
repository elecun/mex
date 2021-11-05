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
 Task Reservation model
'''
class Reserve(models.Model):
    uid = models.CharField(max_length=64, blank=False, null=False, default=uuid.uuid4().hex, unique=True)
    start_at = models.DateTimeField()

    def __str__(self):
        return self.uid


'''
Settings parameter model
'''
class Settings(models.Model):
    uid = models.CharField(max_length=64, blank=False, null=False, default=uuid.uuid4().hex, unique=True)
    name = models.CharField(max_length=255, blank=False, default="")
    created_at = models.DateTimeField(auto_now_add=True)
    note = models.TextField(blank=True, null=True)

    machine_name = models.CharField(max_length=255, blank=True, default="") #장비명
    jsno = models.CharField(max_length=255, blank=True, default="") #JS No
    product_size = models.FloatField(blank=True, null=True, default=0.0) #제품 사이즈
    vload = models.FloatField(blank=True, null=True, default=0.0) #수직하중
    roller_size = models.FloatField(blank=True, null=True, default=0.0) #구동롤러 사이즈
    wtime = models.FloatField(blank=True, null=True, default=0.0) #시험시간
    update_interval = models.PositiveIntegerField(blank=True, null=True, default=10) # 측정주기
    limit_temperature_min = models.IntegerField(blank=True, null=True, default=0) #제한온도
    limit_temperature_max = models.IntegerField(blank=True, null=True, default=100)
    limit_temperature_min_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_temperature_max_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_rpm_max = models.PositiveIntegerField(blank=True, null=True, default=0) #제한속도
    limit_rpm_max_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_load_min = models.FloatField(blank=True, null=True, default=0.0) #제한하중
    limit_load_max = models.FloatField(blank=True, null=True, default=0.0)
    limit_load_min_count = models.PositiveIntegerField(blank=True, null=True, default=3)
    limit_load_max_count = models.PositiveIntegerField(blank=True, null=True, default=3)

    steps = models.TextField(blank=True, null=True) # 상세설정

    reserve_id = models.ForeignKey("Reserve", null=True, on_delete=models.SET_NULL, db_column="reserve_id", related_name="reserve")

    def __str__(self):
        return self.name



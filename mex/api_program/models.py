
#-*- coding:utf-8 -*-
from django.db import models
from django.utils.translation import gettext as _
import uuid

# program command
class Command(models.Model):
    command = models.CharField(max_length=16, blank=False, null=False, default="STOP")
    
    def __str__(self):
        return self.command


# program step
class ProgramStep(models.Model):
    step = models.PositiveIntegerField(default=1, unique=False)
    command = models.ForeignKey(Command, null=True, on_delete=models.SET_NULL)
    duration = models.PositiveIntegerField(default=1)
    rpm = models.PositiveIntegerField(default=100)
    sideload = models.BooleanField(default=False)
    acc = models.PositiveIntegerField(default=10)
    dec = models.PositiveIntegerField(default=10)

    def __str__(self):
        return self.step

# program
class Program(models.Model):
    id = models.CharField(max_length=64, blank=False, null=False, default=uuid.uuid4().hex, unique=True, primary_key=True)
    name = models.CharField(max_length=255, blank=True, default="")
    step = models.ForeignKey(ProgramStep, null=True, on_delete=models.SET_NULL)

    def __str__(self):
        return self.program_name



# general settings
class Settings(models.Model):
    machine_name = models.CharField(max_length=256, blank=False, null=False)
    jsno = models.CharField(max_length=128, blank=False, null=False)
    product_size = models.FloatField()
    load = models.FloatField()
    roller_size = models.FloatField()
    exp_time = models.FloatField()
    update_period = models.PositiveIntegerField(default=10)
    limit_temperature_min = models.IntegerField(default=0)
    limit_temperature_max = models.IntegerField(default=100)
    limit_tenperature_min_count = models.PositiveIntegerField(default=3)
    limit_rpm = models.PositiveIntegerField()
    limit_rpm_count = models.PositiveIntegerField(default=3)
    limit_load_min = models.FloatField()
    limit_load_max = models.FloatField()
    limit_load_min_count = models.PositiveIntegerField(default=3)
    limit_load_max_count = models.PositiveIntegerField(default=3)
    def __str__(self):
        return id
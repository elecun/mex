run:
	@python3 manage.py runserver 0.0.0.0:8000

migrate:
	@python3 manage.py migrate --run-syncdb

loaddata:
	@python3 manage.py loaddata command.json
#	@python3 manage.py loaddata apptype.json

crontab_remove:
	@python3 manage.py crontab remove

crontab_add:
	@python3 manage.py crontab add

superuser:
	@python3 manage.py createsuperuser

#run-celery-worker:
#	@celery -A django_graph worker -l info

#run-celery-beat:
#	@celery -A django_graph beat -l info

#run-redis:
#	redis-server --daemonize yes
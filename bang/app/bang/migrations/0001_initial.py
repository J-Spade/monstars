# Generated by Django 5.0.6 on 2024-07-13 17:33

from django.db import migrations, models


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='LogonCredential',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('domain', models.CharField(max_length=260)),
                ('username', models.CharField(max_length=260)),
                ('password', models.CharField(max_length=260)),
                ('last_used', models.DateTimeField(blank=True, null=True)),
                ('last_changed', models.DateTimeField(blank=True, null=True)),
                ('last_hostname', models.CharField(blank=True, default='', max_length=253)),
            ],
        ),
        migrations.CreateModel(
            name='AuthenticationToken',
            fields=[
                ('token', models.UUIDField(primary_key=True, serialize=False)),
                ('revoked', models.BooleanField(default=False)),
                ('last_used', models.DateTimeField(blank=True, null=True)),
                ('last_hostname', models.CharField(blank=True, default='', max_length=253)),
            ],
        ),
    ]
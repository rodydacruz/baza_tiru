extends Node2D

signal score(points, enemy_id)
signal alert(position, enemy_id)
var enemy_id : int = randi() 

var current_phase = Settings.Phase.APEAR
var speed = 60
var strike_count = 0;
var totalHealth = 1000.0
var health = totalHealth
var explosion_scene = preload("res://scenes/explosion.tscn")




func _process(delta):
	if(get_parent().is_game_over != true):
		movementByFase(delta)
	pass
	


func _on_atack_timer_timeout() -> void:
	atackOntimerByFase()
	
	
func movementByFase(delta):
	var stage = get_parent()
	if(!stage.is_game_over):
		match current_phase:
			Settings.Phase.APEAR:
				#Vector2(SCREEN_WIDTH + 8, randf_range(0,SCREEN_HEIGHT))
				if(position.x > Settings.BOSS_ATACK_POSITION):
					position.x = position.x - (delta * speed)
				
				
				var player_position = stage.getPlayerPosition()
				var diff =  position.y - player_position.y
				position.y = position.y + ((1 if diff < 0 else -1) *  (delta * speed )) 
				pass
			Settings.Phase.HIDE:
				if(position.x > Settings.BOSS_LEFT_HIDE_POSITION):
					position.x = position.x - (delta * speed * 5 )
				if(strike_count >= 10):
					current_phase = Settings.Phase.AlL_IN
					
					
			Settings.Phase.AlL_IN: 
				if(position.x < Settings.BOSS_LEFF_ATACK_POSITION):
					position.x = position.x + (delta * speed * 5 )
					rotation_degrees = 180
				
			_:
				pass
func atackOntimerByFase():
	var stage = get_parent()
	if(!stage.is_game_over):
		match current_phase:
			Settings.Phase.APEAR:
				shootCanonBall()
			Settings.Phase.HIDE:
				shootCanonBall()
				pass
			_:
				pass
func shootCanonBall():
	var stage = get_parent()
	if(!stage.is_game_over):
		var CannonBall = preload("res://scenes/canon_ball.tscn")
		var canon_ball_instance  = CannonBall.instantiate()
		canon_ball_instance.global_position = Vector2(position.x - 20,position.y)
		
		# Get Payler Possition and Vector Pointed to it
		var player_possition = stage.getPlayerPosition()
		
		canon_ball_instance.vector = Vector2((player_possition - position)) 
		
		get_parent().add_child(canon_ball_instance)
		if current_phase == Settings.Phase.HIDE:
			strike_count += 1;
			canon_ball_instance.phase = Settings.Phase.HIDE
			var TWarning = preload("res://scenes/t_warning.tscn")
			var t_warning_instance  = TWarning.instantiate()
			t_warning_instance.threat  = canon_ball_instance;
			t_warning_instance.global_position.y = canon_ball_instance.global_position.y
			get_parent().add_child(t_warning_instance)
	
func _draw_health() -> void:
	var health_bar  = get_node("health_bar") as ProgressBar
	var per = (health/totalHealth)*100
	print("Perce", per)
	health_bar.set_value_no_signal(per)
	pass
func startHitAnimation():
	position.x += 2; 
	pass

func sprintOffScreen():
	pass

func expload(position):
	var stage_node = get_parent()
	
	var explosion_instance = explosion_scene.instantiate()
			
	explosion_instance.position = position
	stage_node.add_child(explosion_instance)
	
func _on_area_entered(area: Area2D) -> void:
	if area.is_in_group("shot"):
		if health > 0:
			emit_signal("score", 10, enemy_id)
			health -= 10
			startHitAnimation()
			expload(area.position) 
			if(health == totalHealth*0.7 ):
				current_phase = Settings.Phase.HIDE;
			
			_draw_health()
		else:
			expload(area.position)
			queue_free()
	elif area.is_in_group("player"):
		expload(area.position) 
			

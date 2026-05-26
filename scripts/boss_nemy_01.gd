extends Node2D

signal score(points, enemy_id)
signal alert(position, enemy_id)
var enemy_id : int = randi() 
enum Phase {
	APEAR,
	HIDE,
	AlL_IN,
	DEFEATED
	
}
var current_phase = Phase.APEAR
var speed = 60
var health = 1000
var explosion_scene = preload("res://scenes/explosion.tscn")




func _process(delta):
	if(get_parent().is_game_over != true):
		movementByFase(delta,current_phase)
	pass
	


func _on_atack_timer_timeout() -> void:
	shootCanonBall()
	
func movementByFase(delta,fase):
	var stage = get_parent()
	if(!stage.is_game_over):
		match fase:
			Phase.APEAR:
				#Vector2(SCREEN_WIDTH + 8, randf_range(0,SCREEN_HEIGHT))
				if(position.x > Settings.BOST_ATACK_POSITION):
					position.x = position.x - (delta * speed)
				
				
				var player_position = stage.getPlayerPosition()
				var diff =  position.y - player_position.y
				position.y = position.y + ((1 if diff < 0 else -1) *  (delta * speed )) 
				pass
			_:
				pass
func atackOntimerByFase():
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
	
	
func startHitAnimation():
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
		
		else:
			expload(area.position)
			#queue_free()
			

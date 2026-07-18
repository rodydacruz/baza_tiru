extends Control


var stage
var threat = null

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	stage = get_parent()
	
	
	
	
	pass # Replace with function body.

func getAngleBetweenTwoVector(vector1, vector2):
	var arrow = vector2 - vector1
	var down = Vector2(0, 1)
	
	# Vector2 has a built-in method to find the angle between two vectors
	var radians = arrow.angle_to(down)
	var degrees = abs(rad_to_deg(radians)) # Use abs() if you want a positive angle
	print(degrees)
	return degrees

func move_to_the_middle(delta,v1,v2):
	var v = v1 - v2
	position.y = v2.y + v.y *(0.5)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	
	if threat != null && threat.position.x < -10  && !stage.is_game_over:
		var player_possition = stage.getPlayerPosition()
		move_to_the_middle(delta,player_possition,threat.position)
		
		rotation_degrees = -getAngleBetweenTwoVector(threat.position, position)
	else:
		queue_free()
		
	pass
